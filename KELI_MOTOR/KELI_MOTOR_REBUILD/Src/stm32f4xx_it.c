/* Interrupt Service Routines — STM32F407 */

#include "main.h"
#include "stm32f4xx_it.h"
#include "adc_reg.h"
#include "spi_Reg.h"
#include "pid.h"
#include "encoder_reg.h"
#include "motor_fsm.h"

/* ---- Control parameters ---- */
#define MOTOR_MAX_RPM            100.0f   /* potentiometer full scale -> RPM */
#define MOTOR_MIN_RUN_RPM        15.0f
/* START->RUN handoff threshold. Lower than MIN_RUN_RPM so the speed PID
 * has headroom to hold the motor once closed-loop takes over. */
#define MOTOR_START_COMPLETE_RPM 12.0f
#define MOTOR_STOP_ADC_MAX       250U     /* hysteresis: stop below this */
#define MOTOR_START_ADC_MIN      350U     /* hysteresis: start above this */
#define SPEED_LOOP_DIVIDER       10U      /* speed loop runs at 100Hz (every 10ms) */
#define DUTY_SLEW_PER_MS         2U
#define DUTY_STOP_SLEW_PER_MS   4U
#define MOTOR_MAX_CURRENT_MA     400.0f

/* START-phase anti-windup:
 * Without these, static friction keeps RPM=0 while speed integral grows freely.
 * When the motor breaks free it overshoots by ~140%.
 *   ISET_MAX: hard cap on current setpoint during START (primary limiter)
 *   INTEGRAL_MAX: second safety cap on integral accumulation
 *   HANDOFF_KEEP: fraction of speed integral kept when transitioning to RUN */
#define SPEED_START_ISET_MAX       110.0f
#define SPEED_START_INTEGRAL_MAX   11000.0f
#define SPEED_HANDOFF_KEEP         0.4f

/* At START->RUN handoff, bleed current loop integral and duty accumulator
 * to release energy trapped by the duty slew limiter. */
#define CURRENT_HANDOFF_KEEP       0.4f
#define DUTY_HANDOFF_KEEP          0.6f


/* ---- Module-level state ---- */
PID_TypeDef speed_pid;
PID_TypeDef current_pid;
volatile uint16_t  g_pid_duty = 0U;
volatile uint16_t  g_adc_val  = 0U;
volatile float     g_current_setpoint_mA = 0.0f;

volatile MotorState     g_motor_state     = MOTOR_INIT;
volatile uint32_t       g_motor_faults    = MOTOR_FAULT_NONE;
volatile MotorTelemetry g_motor_telemetry = {0};

/* TIM6: APB1=42MHz, APB1 prescaler!=1 -> TIM_CLK=84MHz
 * PSC=83, ARR=999 -> 84MHz/84/1000 = 1kHz */
void Motor_Control_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->PSC  = 83U;
    TIM6->ARR  = 999U;
    TIM6->CNT  = 0U;
    TIM6->DIER |= TIM_DIER_UIE;
    NVIC_SetPriority(TIM6_DAC_IRQn, 2U);
    NVIC_EnableIRQ(TIM6_DAC_IRQn);
    TIM6->CR1 |= TIM_CR1_CEN;

    /* Speed loop: setpoint=RPM, output=current_setpoint(mA)
     * integral_max = out_max/ki = 400/0.01 = 40000 */
    PID_Init(&speed_pid,
             0.8f, 0.01f, 0.0f,
             0.0f, MOTOR_MAX_CURRENT_MA, 40000.0f);

    /* Current loop: setpoint=mA, output=PWM duty (0-4095)
     * Must be 3-5x faster than speed loop. integral_max=8000 */
    PID_Init(&current_pid,
             5.0f, 0.2f, 0.0f,
             0.0f, 4095.0f, 8000.0f);

    Encoder_Init();
    g_motor_state = MOTOR_IDLE;
}

/* ---- 1kHz control loop ---- */
void TIM6_DAC_IRQHandler(void)
{
    static uint8_t  speed_loop_count = 0U;
    static uint16_t duty_applied     = 0U;

    if (!(TIM6->SR & TIM_SR_UIF)) { return; }
    TIM6->SR &= ~TIM_SR_UIF;

    /* 1. Read inputs */
    uint16_t adc_val = ADC1_Read_Filtered();
    g_adc_val = adc_val;

    float setpoint_rpm = 0.0f;
    if (adc_val >= MOTOR_START_ADC_MIN) {
        setpoint_rpm = MOTOR_MIN_RUN_RPM
                     + (float)(adc_val - MOTOR_START_ADC_MIN)
                       * ((MOTOR_MAX_RPM - MOTOR_MIN_RUN_RPM)
                          / (4095.0f - (float)MOTOR_START_ADC_MIN));
    }

    float measured_current = ADC1_ReadCurrent_Filtered_mA();

    speed_loop_count++;
    uint8_t speed_loop_due = 0U;
    if (speed_loop_count >= SPEED_LOOP_DIVIDER) {
        speed_loop_count = 0U;
        Encoder_Update();
        speed_loop_due = 1U;
    }

    uint8_t stopped = (g_encoder_rpm > -0.5f && g_encoder_rpm < 0.5f) ? 1U : 0U;

    /* 2. State transitions */
    switch (g_motor_state) {
    case MOTOR_INIT:
        g_motor_state = MOTOR_IDLE;
        break;
    case MOTOR_IDLE:
        if (adc_val >= MOTOR_START_ADC_MIN) { g_motor_state = MOTOR_STARTING; }
        break;
    case MOTOR_STARTING:
        if (adc_val <= MOTOR_STOP_ADC_MAX) {
            g_motor_state = MOTOR_STOPPING;
        } else if (g_encoder_rpm >= MOTOR_START_COMPLETE_RPM) {
            /* Bleed accumulated energy before handing off to RUN to reduce overshoot */
            speed_pid.integral   *= SPEED_HANDOFF_KEEP;
            current_pid.integral *= CURRENT_HANDOFF_KEEP;
            duty_applied = (uint16_t)((float)duty_applied * DUTY_HANDOFF_KEEP);
            g_motor_state = MOTOR_RUNNING;
        }
        break;
    case MOTOR_RUNNING:
        if (adc_val <= MOTOR_STOP_ADC_MAX) { g_motor_state = MOTOR_STOPPING; }
        break;
    case MOTOR_STOPPING:
        if (adc_val >= MOTOR_START_ADC_MIN) {
            g_motor_state = MOTOR_STARTING;
        } else if (duty_applied == 0U && stopped) {
            g_motor_state = MOTOR_IDLE;
        }
        break;
    case MOTOR_FAULT:
    default:
        break;
    }

    /* 3. Actions */
    uint16_t duty = 0U;

    switch (g_motor_state) {
    case MOTOR_STARTING:
    case MOTOR_RUNNING: {
        if (setpoint_rpm < MOTOR_MIN_RUN_RPM) { setpoint_rpm = MOTOR_MIN_RUN_RPM; }

        if (speed_loop_due) {
            g_current_setpoint_mA = PID_Calc(&speed_pid, setpoint_rpm, g_encoder_rpm);

            if (g_motor_state == MOTOR_STARTING) {
                if (speed_pid.integral > SPEED_START_INTEGRAL_MAX) {
                    speed_pid.integral = SPEED_START_INTEGRAL_MAX;
                }
                if (g_current_setpoint_mA > SPEED_START_ISET_MAX) {
                    g_current_setpoint_mA = SPEED_START_ISET_MAX;
                }
            }
        }

        uint16_t duty_target;
        if (g_current_setpoint_mA <= 0.0f) {
            PID_Reset(&current_pid);
            duty_target = 0U;
        } else {
            duty_target = (uint16_t)PID_Calc(&current_pid, g_current_setpoint_mA, measured_current);
        }

        if      (duty_target > duty_applied + DUTY_SLEW_PER_MS)         { duty_applied += DUTY_SLEW_PER_MS; }
        else if (duty_target + DUTY_SLEW_PER_MS < duty_applied)          { duty_applied -= DUTY_SLEW_PER_MS; }
        else                                                              { duty_applied  = duty_target; }
        duty = duty_applied;
        break;
    }

    case MOTOR_STOPPING: {
        PID_Reset(&speed_pid);
        PID_Reset(&current_pid);
        g_current_setpoint_mA = 0.0f;
        if (duty_applied > DUTY_STOP_SLEW_PER_MS) { duty_applied -= DUTY_STOP_SLEW_PER_MS; }
        else                                       { duty_applied  = 0U; }
        duty = duty_applied;

        if (duty_applied == 0U && stopped) { g_motor_current_mA = 0.0f; }
        break;
    }

    default:
        PID_Reset(&speed_pid);
        PID_Reset(&current_pid);
        g_current_setpoint_mA = 0.0f;
        duty_applied  = 0U;
        duty          = 0U;
        if (stopped) { g_motor_current_mA = 0.0f; }
        break;
    }

    /* 4. Send duty to FPGA via SPI2 (2 bytes, 12-bit value) */
    uint8_t hi = (uint8_t)((duty >> 8U) & 0x0FU);
    uint8_t lo = (uint8_t)(duty & 0xFFU);
    FPGA_CS_LOW();
    SPI2_ReadWriteByte(hi);
    SPI2_ReadWriteByte(lo);
    FPGA_CS_HIGH();

    /* 5. Update telemetry (all fields written atomically in one ISR period) */
    g_pid_duty = duty;
    g_motor_telemetry.state          = g_motor_state;
    g_motor_telemetry.rpm_set        = setpoint_rpm;
    g_motor_telemetry.rpm            = g_encoder_rpm;
    g_motor_telemetry.current_set_mA = g_current_setpoint_mA;
    g_motor_telemetry.current_mA     = g_motor_current_mA;
    g_motor_telemetry.duty           = duty;
    g_motor_telemetry.pot_adc        = adc_val;
    g_motor_telemetry.current_raw    = g_motor_current_raw;
    g_motor_telemetry.faults         = g_motor_faults;
}

/* ---- Cortex-M4 fault handlers ---- */
void NMI_Handler(void)         { while (1) {} }
void HardFault_Handler(void)   { while (1) {} }
void MemManage_Handler(void)   { while (1) {} }
void BusFault_Handler(void)    { while (1) {} }
void UsageFault_Handler(void)  { while (1) {} }
void SVC_Handler(void)         {}
void DebugMon_Handler(void)    {}
void PendSV_Handler(void)      {}

void SysTick_Handler(void)
{
    HAL_IncTick();
}
