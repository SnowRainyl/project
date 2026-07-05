#ifndef __MOTOR_FSM_H
#define __MOTOR_FSM_H

#include <stdint.h>
#include "stm32f4xx.h"     /* NVIC_xxxIRQ，用于遥测原子拷贝 */

/* ============================================================================
 * 电机控制状态机 + 统一遥测快照
 *
 * 设计动机：
 *   1kHz 控制环（TIM6_DAC_IRQHandler）原先用一个 motor_enabled 标志 +
 *   多处散落的 if 来表达"零位/启动/运行/平滑停止"这些隐含状态。本模块把它们
 *   收敛为一个显式状态机，统一管理：
 *     - 电位器启停滞回
 *     - PID 复位时机
 *     - duty 平滑升/降
 *     - iset=0 的处理
 *     - 后续故障停机入口（FAULT，第三阶段填充触发条件）
 *
 *   同时提供一个 MotorTelemetry 快照：ISR 在每个控制周期末尾一次性写入，
 *   读取端（main 监控循环）通过 Motor_GetTelemetry() 原子拷贝整个结构，
 *   避免 RPM / duty / 电流等字段来自不同控制周期导致的瞬时不一致。
 *
 * 状态迁移图：
 *
 *     INIT ──▶ IDLE ──▶ STARTING ──▶ RUNNING
 *                ▲          │             │
 *                │          ▼             ▼
 *                └──────── STOPPING ◀─────┘
 *
 *     任意状态 ──▶ FAULT （第三阶段实现）
 * ============================================================================ */

typedef enum {
    MOTOR_INIT = 0,   /* 上电初始态，下一拍进入 IDLE */
    MOTOR_IDLE,       /* 电位器在零区，电机停止，duty=0，PID 保持复位 */
    MOTOR_STARTING,   /* 检测到启动给定，duty 爬升，尚未达到运行转速 */
    MOTOR_RUNNING,    /* 转速达标，正常串级闭环运行 */
    MOTOR_STOPPING,   /* 检测到停止给定，duty 平滑降至 0 */
    MOTOR_FAULT       /* 故障锁存，强制 duty=0（退出条件待第三阶段实现） */
} MotorState;

/* ---- 故障位（第三阶段使用，当前恒为 0） ---- */
#define MOTOR_FAULT_NONE         0x00000000UL
#define MOTOR_FAULT_OVERCURRENT  (1UL << 0)   /* 过流 */
#define MOTOR_FAULT_STALL        (1UL << 1)   /* 堵转：给定有效但长时间无转速 */
#define MOTOR_FAULT_ENCODER      (1UL << 2)   /* 编码器异常 */

/* ---- 统一遥测快照 ---- */
typedef struct {
    MotorState state;            /* 当前状态机状态 */
    float      rpm_set;          /* 电位器目标转速（RPM，已含启动下限钳位，ramp 终点） */
    float      rpm_set_ramped;   /* 经斜率限幅后实际喂给速度 PID 的目标（RPM） */
    float      rpm;              /* 实测转速（RPM） */
    float      current_set_mA;   /* 速度外环输出的电流给定（mA） */
    float      current_mA;       /* 实测电流（mA） */
    uint16_t   duty;             /* 实际下发 FPGA 的占空比（0~4095） */
    uint16_t   pot_adc;          /* 电位器原始 ADC（0~4095） */
    uint16_t   current_raw;      /* 电流通道原始 ADC（诊断用，0~4095） */
    uint32_t   faults;           /* 故障位（MOTOR_FAULT_xxx 的或） */
} MotorTelemetry;

/* ---- ISR 维护的全局量（定义在 stm32f4xx_it.c） ---- */
extern volatile MotorState     g_motor_state;
extern volatile uint32_t       g_motor_faults;
extern volatile MotorTelemetry g_motor_telemetry;

/**
 * @brief  状态枚举 → 短字符串，用于串口/OLED 显示
 */
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

/**
 * @brief  原子读取遥测快照
 * @param  out  目标结构体指针
 *
 * 拷贝期间临时关闭 TIM6 更新中断，保证读到的是同一个控制周期写入的整组数据，
 * 而不是被 ISR 在拷贝中途改写的"撕裂"组合。拷贝仅 ~32 字节，关中断时间极短，
 * 不影响 1kHz 控制环。
 *
 * 注意：保存并恢复 TIM6 中断原使能状态，避免在 TIM6 本就关闭时被本函数意外打开。
 */
static inline void Motor_GetTelemetry(MotorTelemetry *out)
{
    uint32_t was_enabled = NVIC_GetEnableIRQ(TIM6_DAC_IRQn);
    NVIC_DisableIRQ(TIM6_DAC_IRQn);
    *out = *(const MotorTelemetry *)&g_motor_telemetry;
    if (was_enabled) {
        NVIC_EnableIRQ(TIM6_DAC_IRQn);
    }
}

#endif /* __MOTOR_FSM_H */
