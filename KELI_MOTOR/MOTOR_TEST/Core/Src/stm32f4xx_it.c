/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    stm32f4xx_it.c
  * @brief   Interrupt Service Routines.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx_it.h"
/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "adc_reg.h"
#include "spi_Reg.h"
#include "pid.h"
#include "encoder_reg.h"
#include "motor_fsm.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN TD */

/* USER CODE END TD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/** 电位器满量程对应的目标转速（RPM），按实际电机最高输出轴转速修改 */
/* 电位器满量程对应的目标转速上限（RPM）。
 * 电机铭牌额定 280 RPM，此处限速 100 RPM（约 36% 额定转速）。 */
#define MOTOR_MAX_RPM           100.0f
#define MOTOR_MIN_RUN_RPM       15.0f
/* START→RUN 转移阈值，低于 MIN_RUN_RPM 以提早交接、减少过冲。
 * 下限约束：交接后 kp*(MIN_RUN_RPM - START_COMPLETE_RPM) 须足以维持电机转动。
 * 当前电机 ~20RPM 需约 30mA，kp=0.8 → P项须≥30mA → 误差≥37RPM → 低速目标不能设太低。
 * 建议：低速目标(<25RPM)留 12RPM，高速目标(>35RPM)可再降；此处取保守值 12RPM。 */
#define MOTOR_START_COMPLETE_RPM 12.0f
#define MOTOR_STOP_ADC_MAX      250U
#define MOTOR_START_ADC_MIN     350U
#define SPEED_LOOP_DIVIDER      10U
#define DUTY_SLEW_PER_MS        2U
#define DUTY_STOP_SLEW_PER_MS   4U

/**
 * 速度外环输出的最大电流给定值（mA）
 * 根据 JGA25-370 额定电流和 INA240 + 采样电阻量程设置
 * 超出此值会被速度 PID 积分限幅截断
 */
/* 实测：电机运行时电流 200~300mA，取 400mA 作为内环上限（留裕量） */
#define MOTOR_MAX_CURRENT_MA    400.0f

/* ===== 启动防超调（START 阶段速度积分管理）=====
 *
 * 现象（见 report/测试数据/t2.md）：低速给定（如 27.4 RPM）启动时，电机静摩擦
 * 使 RPM 长时间为 0，速度外环持续累加误差但 iset 远未达 out_max=400mA，PID 自带
 * 的输出饱和 anti-windup 不触发 → 积分膨胀到 ~7400，电机一旦挣脱静摩擦即带着这份
 * 蓄能冲过头，实测超调约 142%（27.4→66.2 RPM）。
 *
 * 三项针对性限制（默认值为起点，需带载/不同档位上板整定）：
 *   ①SPEED_START_ISET_MAX     —— START 阶段电流给定上限（mA，单位明确，与 ki 无关），
 *                                 作为启动电流的“主限制”，>击穿静摩擦所需(~96mA)留裕量
 *   ②SPEED_START_INTEGRAL_MAX —— START 阶段速度积分上限，作为“第二层安全闸”防无限积累
 *   ③SPEED_HANDOFF_KEEP       —— START→RUN 交接时保留的积分比例，泄掉启动多余蓄能
 *
 * 职责划分（重要，见调试记录）：积分钳的实际 mA 效果 = ki×积分钳，会随在线调 ki 漂移；
 * 因此必须让积分钳足够高，使 ①(ISET_MAX，单位 mA) 成为真正生效的主闸，②只兜底防积分
 * 无限膨胀。实测 kp=0.08、ki=0.01 时，旧值 8000 让 iset 仅达 0.08×45+0.01×8000≈84mA，
 * 低于挣脱静摩擦所需(~96mA)，导致 45RPM 堵转启动失败。抬到 11000 后 45RPM 堵转算得
 * iset≈113.6mA → 由 ①(110mA) 接管，ISET_MAX 重新成为主限制。
 */
#define SPEED_START_ISET_MAX       110.0f
#define SPEED_START_INTEGRAL_MAX   11000.0f
#define SPEED_HANDOFF_KEEP         0.4f

/* ===== 启动防超调（START→RUN 电流环 + 执行器交接）=====
 *
 * 现象（见 report/测试数据/t23.md）：当静摩擦较大、电机卡到较高 iset(~82mA)才挣脱时，
 * 即便 ③ 已把速度环 iset 砍低，duty 仍冲到 1182、把 RPM 顶到 44(超调 ~47%)。根因是
 * 电流环积分在击穿瞬间(curr 因反电动势骤降、curr<iset)被顶高，叠加 duty 只能按 2/ms
 * 缓慢回落，启动能量被困在电流环积分与已蓄 duty 里，速度环的 ③ 管不到这部分。
 *
 *   ④CURRENT_HANDOFF_KEEP —— 交接时保留的电流环积分比例，泄掉电流环蓄能
 *   ⑤DUTY_HANDOFF_KEEP    —— 交接时保留的已蓄 duty 比例，释放被下行斜率困住的执行器能量
 */
#define CURRENT_HANDOFF_KEEP       0.4f
#define DUTY_HANDOFF_KEEP          0.6f

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */
PID_TypeDef speed_pid;    /* 速度外环：setpoint=RPM → output=电流给定(mA) */
PID_TypeDef current_pid;  /* 电流内环：setpoint=mA  → output=PWM占空比     */
volatile uint16_t  g_pid_duty = 0;   /* 供 main.c 读取，用于串口/OLED 显示 */
volatile uint16_t  g_adc_val  = 0;   /* 电位器原始 ADC 值（0~4095） */
volatile float g_current_setpoint_mA = 0.0f;

/* ---- 状态机 + 遥测（声明见 motor_fsm.h） ---- */
volatile MotorState     g_motor_state     = MOTOR_INIT;
volatile uint32_t       g_motor_faults    = MOTOR_FAULT_NONE;
volatile MotorTelemetry g_motor_telemetry = {0};

uint16_t sendtofpga;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
void Motor_Control_Init(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* =============================================================================
 * Motor_Control_Init
 *   初始化 TIM6（1kHz 定时器）和 PID 参数
 *   在 main() 的初始化段调用一次
 *
 * TIM6 时钟链路：
 *   APB1_CLK = 42MHz，APB1 预分频 ≠ 1 → TIM6_CLK = 84MHz
 *   PSC = 83, ARR = 999 → 84MHz / 84 / 1000 = 1kHz (1ms 周期)
 *
 * PID 初始参数（无编码器阶段，kp=1 ki=0 kd=0 = 直通，输出=setpoint）：
 *   接入反馈后再逐步调节 kp/ki/kd。
 * ============================================================================= */
void Motor_Control_Init(void)
{
    /* ---- TIM6 时钟使能（APB1） ---- */
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    /* ---- 计数配置 ---- */
    TIM6->PSC = 83;           /* 84MHz / (83+1) = 1MHz       */
    TIM6->ARR = 999;          /* 1MHz / (999+1) = 1kHz       */
    TIM6->CNT = 0;

    /* ---- 使能更新中断 ---- */
    TIM6->DIER |= TIM_DIER_UIE;

    /* ---- NVIC 配置：优先级 2，允许被 SysTick(15) 打断 ---- */
    NVIC_SetPriority(TIM6_DAC_IRQn, 2);
    NVIC_EnableIRQ(TIM6_DAC_IRQn);

    /* ---- 启动计数器 ---- */
    TIM6->CR1 |= TIM_CR1_CEN;

    /* ---- 串级 PID 参数初始化 ---- */
    /*
     * 速度外环（Speed Loop）
     *   setpoint : 目标转速（RPM，电位器给定，0~MOTOR_MAX_RPM）
     *   feedback : 编码器实测转速 g_encoder_rpm（RPM）
     *   output   : 期望电流给定（mA），送入电流内环
     *
     *   实测电流范围：75mA（最低速）~ 135mA（最高速）
     *   out_max = MOTOR_MAX_CURRENT_MA = 150mA
     *
     *   kp=0.8 → 兼顾启动响应与测速量化误差
     *   ki=0.01 → 加快消除稳态转速误差，同时避免低频振荡
     *   kd=0.0 → 编码器信号含噪，省略微分
     *   integral_max = out_max / ki = 400 / 0.01 = 40000
     */
    PID_Init(&speed_pid,
             /*kp*/0.8f,  /*ki*/0.01f,  /*kd*/0.0f,
             /*out_min*/0.0f,  /*out_max*/MOTOR_MAX_CURRENT_MA,
             /*integral_max*/40000.0f);

    /*
     * 电流内环（Current Loop）
     *   setpoint : 期望电流（mA，速度外环输出，0~150mA）
     *   feedback : INA240 实测电流 g_motor_current_mA（mA）
     *   output   : PWM 占空比（0~4095，送 FPGA）
     *
     *   内环须比外环快 3~5 倍，用较大 kp 加快电流响应。
     *   kp=20  → 150mA 误差时 P 项 = 3000（73% duty），快速响应
     *   ki=0.5 → 稳态 duty≈2482 所需积分 = 2482/0.5 = 4964 < integral_max=8000 ✓
     *   kd=0.0 → 电流信号较干净，省略微分（需要时可加 0.01~0.05）
     *   integral_max = out_max / ki = 4095 / 0.5 = 8190，取 8000
     */
    PID_Init(&current_pid,
             /*kp*/5.0f,  /*ki*/0.2f,  /*kd*/0.0f,  /* kp 从20降到5，避免振荡 */
             /*out_min*/0.0f,  /*out_max*/4095.0f,
             /*integral_max*/8000.0f);

    /* ---- 编码器初始化（TIM3，PC6/PC7） ---- */
    Encoder_Init();

    /*
     * 状态机初始化为 IDLE：MOTOR_INIT 仅代表 TIM6 启动前的上电态。
     * 在此显式落到 IDLE，使第一拍 TIM6 中断即可根据 adc 直接进入 STARTING，
     * 与改造前"上电后首拍即响应电位器"的行为完全一致，无额外 1ms 死拍。
     */
    g_motor_state = MOTOR_IDLE;
}

/* USER CODE END 0 */

/* External variables --------------------------------------------------------*/

/* USER CODE BEGIN EV */

/* USER CODE END EV */

/******************************************************************************/
/*           Cortex-M4 Processor Interruption and Exception Handlers          */
/******************************************************************************/
/**
  * @brief This function handles Non maskable interrupt.
  */
void NMI_Handler(void)
{
  /* USER CODE BEGIN NonMaskableInt_IRQn 0 */

  /* USER CODE END NonMaskableInt_IRQn 0 */
  /* USER CODE BEGIN NonMaskableInt_IRQn 1 */
   while (1)
  {
  }
  /* USER CODE END NonMaskableInt_IRQn 1 */
}

/**
  * @brief This function handles Hard fault interrupt.
  */
void HardFault_Handler(void)
{
  /* USER CODE BEGIN HardFault_IRQn 0 */

  /* USER CODE END HardFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_HardFault_IRQn 0 */
    /* USER CODE END W1_HardFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Memory management fault.
  */
void MemManage_Handler(void)
{
  /* USER CODE BEGIN MemoryManagement_IRQn 0 */

  /* USER CODE END MemoryManagement_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_MemoryManagement_IRQn 0 */
    /* USER CODE END W1_MemoryManagement_IRQn 0 */
  }
}

/**
  * @brief This function handles Pre-fetch fault, memory access fault.
  */
void BusFault_Handler(void)
{
  /* USER CODE BEGIN BusFault_IRQn 0 */

  /* USER CODE END BusFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_BusFault_IRQn 0 */
    /* USER CODE END W1_BusFault_IRQn 0 */
  }
}

/**
  * @brief This function handles Undefined instruction or illegal state.
  */
void UsageFault_Handler(void)
{
  /* USER CODE BEGIN UsageFault_IRQn 0 */

  /* USER CODE END UsageFault_IRQn 0 */
  while (1)
  {
    /* USER CODE BEGIN W1_UsageFault_IRQn 0 */
    /* USER CODE END W1_UsageFault_IRQn 0 */
  }
}

/**
  * @brief This function handles System service call via SWI instruction.
  */
void SVC_Handler(void)
{
  /* USER CODE BEGIN SVCall_IRQn 0 */

  /* USER CODE END SVCall_IRQn 0 */
  /* USER CODE BEGIN SVCall_IRQn 1 */

  /* USER CODE END SVCall_IRQn 1 */
}

/**
  * @brief This function handles Debug monitor.
  */
void DebugMon_Handler(void)
{
  /* USER CODE BEGIN DebugMonitor_IRQn 0 */

  /* USER CODE END DebugMonitor_IRQn 0 */
  /* USER CODE BEGIN DebugMonitor_IRQn 1 */

  /* USER CODE END DebugMonitor_IRQn 1 */
}

/**
  * @brief This function handles Pendable request for system service.
  */
void PendSV_Handler(void)
{
  /* USER CODE BEGIN PendSV_IRQn 0 */

  /* USER CODE END PendSV_IRQn 0 */
  /* USER CODE BEGIN PendSV_IRQn 1 */

  /* USER CODE END PendSV_IRQn 1 */
}

/**
  * @brief This function handles System tick timer.
  */
void SysTick_Handler(void)
{
  /* USER CODE BEGIN SysTick_IRQn 0 */

  /* USER CODE END SysTick_IRQn 0 */
  HAL_IncTick();
  /* USER CODE BEGIN SysTick_IRQn 1 */

  /* USER CODE END SysTick_IRQn 1 */
}

/******************************************************************************/
/* STM32F4xx Peripheral Interrupt Handlers                                    */
/* Add here the Interrupt Handlers for the used peripherals.                  */
/* For the available peripheral interrupt handler names,                      */
/* please refer to the startup file (startup_stm32f4xx.s).                    */
/******************************************************************************/

/* USER CODE BEGIN 1 */

/* =============================================================================
 * TIM6 更新中断 —— 1kHz PI 控制环
 *
 * 执行时间估算（5.25MHz SPI，21MHz ADC）：
 *   两路 ADC 转换约 9.33μs，SPI 2字节理论传输时间约 3.05μs。
 *   实际中断时间还包括编码器读取、PI 运算和寄存器操作，需实测确认。
 * ============================================================================= */
void TIM6_DAC_IRQHandler(void)
{
    static uint8_t  speed_loop_count = 0U;
    static uint16_t duty_applied     = 0U;

    if (TIM6->SR & TIM_SR_UIF) {
        uint8_t  speed_loop_due = 0U;
        uint16_t duty           = 0U;

        TIM6->SR &= ~TIM_SR_UIF;    /* 清除更新中断标志（必须第一时间清） */

        /* ---- 1. 采样输入：电位器 → 目标转速（0~4095 → 0~MOTOR_MAX_RPM RPM） ---- */
        uint16_t adc_val = ADC1_Read_Filtered();
        g_adc_val        = adc_val;
        float setpoint_rpm = 0.0f;
        if (adc_val >= MOTOR_START_ADC_MIN) {
            setpoint_rpm = MOTOR_MIN_RUN_RPM
                         + (float)(adc_val - MOTOR_START_ADC_MIN)
                           * ((MOTOR_MAX_RPM - MOTOR_MIN_RUN_RPM)
                              / (4095.0f - (float)MOTOR_START_ADC_MIN));
        }

        /* 电流内环以 1kHz 采样 */
        float measured_current = ADC1_ReadCurrent_Filtered_mA();

        /* 编码器始终保持同步；每 10ms 产生一次速度外环节拍。 */
        speed_loop_count++;
        if (speed_loop_count >= SPEED_LOOP_DIVIDER) {
            speed_loop_count = 0U;
            Encoder_Update();
            speed_loop_due = 1U;
        }

        /* 转轴是否已停（用于停机时清零显示电流、判定 STOPPING→IDLE） */
        uint8_t stopped = (g_encoder_rpm > -0.5f && g_encoder_rpm < 0.5f) ? 1U : 0U;

        /* ---- 2. 状态转移（Mealy：先迁移状态，再按状态执行动作） ----
         *
         * 启停滞回沿用原阈值：adc≥350 启动、adc≤250 停止，避免电位器零位噪声
         * 导致电机间歇启停。STARTING/RUNNING 仅作状态标签，控制动作完全相同。
         */
        switch (g_motor_state) {
        case MOTOR_INIT:
            g_motor_state = MOTOR_IDLE;
            break;
        case MOTOR_IDLE:
            if (adc_val >= MOTOR_START_ADC_MIN) {
                g_motor_state = MOTOR_STARTING;
            }
            break;
        case MOTOR_STARTING:
            if (adc_val <= MOTOR_STOP_ADC_MAX) {
                g_motor_state = MOTOR_STOPPING;
            } else if (g_encoder_rpm >= MOTOR_START_COMPLETE_RPM) {
                /* 启动防超调③：交接到 RUN 前泄掉 START 阶段速度环积累的多余积分，
                 * 避免击穿静摩擦瞬间的蓄能整体带入 RUN 造成冲过头。 */
                speed_pid.integral *= SPEED_HANDOFF_KEEP;
                /* 启动防超调④⑤：电流环 + 执行器交接。强力击穿时(见 t23)电流环积分
                 * 与已蓄 duty 也困着启动能量，③ 管不到；这里在交接同一拍一并泄掉，
                 * 让 RUN 从更低的执行器基线起步。 */
                current_pid.integral *= CURRENT_HANDOFF_KEEP;
                duty_applied = (uint16_t)((float)duty_applied * DUTY_HANDOFF_KEEP);
                g_motor_state = MOTOR_RUNNING;     /* 单向：达到运行转速后不回退 */
            }
            break;
        case MOTOR_RUNNING:
            if (adc_val <= MOTOR_STOP_ADC_MAX) {
                g_motor_state = MOTOR_STOPPING;
            }
            break;
        case MOTOR_STOPPING:
            if (adc_val >= MOTOR_START_ADC_MIN) {
                g_motor_state = MOTOR_STARTING;    /* 减速途中又给定，重新启动 */
            } else if (duty_applied == 0U && stopped) {
                g_motor_state = MOTOR_IDLE;        /* 真正停稳才回 IDLE */
            }
            break;
        case MOTOR_FAULT:
        default:
            /* 故障锁存：保持停机，退出条件留待第三阶段实现 */
            break;
        }

        /* ---- 3. 按状态执行动作 ---- */
        switch (g_motor_state) {
        case MOTOR_STARTING:
        case MOTOR_RUNNING: {
            if (setpoint_rpm < MOTOR_MIN_RUN_RPM) {
                setpoint_rpm = MOTOR_MIN_RUN_RPM;
            }

            /*
             * 编码器测速和速度外环降至 100Hz。
             * 1ms 窗口内一个计数约等于 40RPM，过于离散；10ms 窗口可显著降低量化跳变。
             */
            if (speed_loop_due != 0U) {
                g_current_setpoint_mA = PID_Calc(&speed_pid, setpoint_rpm, g_encoder_rpm);

                /*
                 * START 阶段启动防超调：电机尚未达到运行转速，存在长时间堵转的可能。
                 * 此时速度环 iset 远未到 out_max(400mA)，PID 自带的输出饱和 anti-windup
                 * 不会触发，积分会自由膨胀。这里显式施加两道限制：
                 *   ②夹住积分，防止堵转期间无界蓄能；
                 *   ①限制电流给定上限，限制击穿静摩擦时注入的能量。
                 * 进入 RUN 后这两道限制解除，由原 PID 限幅/anti-windup 接管。
                 */
                if (g_motor_state == MOTOR_STARTING) {
                    if (speed_pid.integral > SPEED_START_INTEGRAL_MAX) {
                        speed_pid.integral = SPEED_START_INTEGRAL_MAX;
                    }
                    if (g_current_setpoint_mA > SPEED_START_ISET_MAX) {
                        g_current_setpoint_mA = SPEED_START_ISET_MAX;
                    }
                }
            }

            /* 电流内环 PID → PWM 占空比（0~4095） */
            uint16_t duty_target;
            if (g_current_setpoint_mA <= 0.0f) {
                PID_Reset(&current_pid);
                duty_target = 0U;
            } else {
                duty_target = (uint16_t)PID_Calc(&current_pid,
                                                 g_current_setpoint_mA,
                                                 measured_current);
            }

            /*
             * 限制占空比每毫秒的变化量，避免电流采样纹波直接变成扭矩冲击。
             * 当前设置从 0 到满占空比约需 2 秒。
             */
            if (duty_target > duty_applied + DUTY_SLEW_PER_MS) {
                duty_applied += DUTY_SLEW_PER_MS;
            } else if (duty_target + DUTY_SLEW_PER_MS < duty_applied) {
                duty_applied -= DUTY_SLEW_PER_MS;
            } else {
                duty_applied = duty_target;
            }
            duty = duty_applied;
            break;
        }

        case MOTOR_STOPPING: {
            PID_Reset(&speed_pid);
            PID_Reset(&current_pid);
            g_current_setpoint_mA = 0.0f;
            /*
             * 进入死区后平滑减小占空比，避免扭矩瞬间消失造成机械卡顿。
             * duty 降到 0 后由状态机迁移到 IDLE，不在死区内持续驱动。
             */
            if (duty_applied > DUTY_STOP_SLEW_PER_MS) {
                duty_applied -= DUTY_STOP_SLEW_PER_MS;
            } else {
                duty_applied = 0U;
            }
            duty = duty_applied;

            if (duty_applied == 0U && stopped) {
                g_motor_current_mA = 0.0f;
            }
            break;
        }

        case MOTOR_IDLE:
        case MOTOR_INIT:
        case MOTOR_FAULT:
        default:
            /*
             * 电位器位于零区 / 故障锁存：直接停机。
             * 若仍运行电流 PID，零点噪声产生的负电流会形成正误差，导致 2%~10% duty 自激。
             */
            PID_Reset(&speed_pid);
            PID_Reset(&current_pid);
            g_current_setpoint_mA = 0.0f;
            duty_applied          = 0U;
            duty                  = 0U;
            if (stopped) {
                g_motor_current_mA = 0.0f;
            }
            break;
        }

        sendtofpga = duty;

        /* ---- 4. 通过 SPI2 发给 FPGA（2字节，12bit 占空比） ---- */
        uint8_t hi = (duty >> 8U) & 0x0FU;
        uint8_t lo =  duty        & 0xFFU;

        FPGA_CS_LOW();
        SPI2_ReadWriteByte(hi);
        SPI2_ReadWriteByte(lo);
        FPGA_CS_HIGH();

        /* ---- 5. 更新监控变量 + 遥测快照（一次写入，保证整组字段同周期一致） ---- */
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
}

/* USER CODE END 1 */
