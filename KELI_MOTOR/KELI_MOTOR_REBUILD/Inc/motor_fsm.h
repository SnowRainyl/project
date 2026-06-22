#ifndef MOTOR_FSM_H
#define MOTOR_FSM_H

#include <stdint.h>
#include "stm32f4xx.h"

/* Motor state machine + unified telemetry snapshot.
 *
 * State transitions:
 *   INIT -> IDLE -> STARTING -> RUNNING
 *                      |            |
 *                      v            v
 *                   STOPPING <------+
 *                      |
 *                      v
 *                    IDLE
 *
 *   Any state -> FAULT (overcurrent / stall / encoder fault) */

typedef enum {
    MOTOR_INIT     = 0,
    MOTOR_IDLE,
    MOTOR_STARTING,
    MOTOR_RUNNING,
    MOTOR_STOPPING,
    MOTOR_FAULT
} MotorState;

#define MOTOR_FAULT_NONE         0x00000000UL
#define MOTOR_FAULT_OVERCURRENT  (1UL << 0)
#define MOTOR_FAULT_STALL        (1UL << 1)
#define MOTOR_FAULT_ENCODER      (1UL << 2)

typedef struct {
    MotorState state;
    float      rpm_set;          /* potentiometer target (RPM), after MIN clamp */
    float      rpm;              /* measured speed (RPM) */
    float      current_set_mA;   /* speed loop output = current setpoint (mA) */
    float      current_mA;       /* measured current (mA) */
    uint16_t   duty;             /* PWM duty sent to FPGA (0-4095) */
    uint16_t   pot_adc;          /* potentiometer raw ADC (0-4095) */
    uint16_t   current_raw;      /* current channel raw ADC (diagnostic) */
    uint32_t   faults;
} MotorTelemetry;

extern volatile MotorState     g_motor_state;
extern volatile uint32_t       g_motor_faults;
extern volatile MotorTelemetry g_motor_telemetry;

static inline const char *Motor_State_Name(MotorState s)
{
    switch (s) {
        case MOTOR_INIT:     return "INIT";
        case MOTOR_IDLE:     return "IDLE";
        case MOTOR_STARTING: return "START";
        case MOTOR_RUNNING:  return "RUN";
        case MOTOR_STOPPING: return "STOP";
        case MOTOR_FAULT:    return "FAULT";
        default:             return "?";
    }
}

/* Atomic snapshot read: briefly disables TIM6 IRQ to prevent torn reads.
 * ~32 bytes copy, interrupt latency is negligible. */
static inline void Motor_GetTelemetry(MotorTelemetry *out)
{
    uint32_t was_enabled = NVIC_GetEnableIRQ(TIM6_DAC_IRQn);
    NVIC_DisableIRQ(TIM6_DAC_IRQn);
    *out = *(const MotorTelemetry *)&g_motor_telemetry;
    if (was_enabled) { NVIC_EnableIRQ(TIM6_DAC_IRQn); }
}

#endif /* MOTOR_FSM_H */
