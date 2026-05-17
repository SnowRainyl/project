/**
 * @file    encoder_reg.h
 * @brief   JGA25-370 编码器驱动（寄存器级）
 *
 * 硬件连接：
 *   编码器 A 相 → PC6  (TIM3_CH1, AF2)
 *   编码器 B 相 → PC7  (TIM3_CH2, AF2)
 *
 * 定时器：TIM3（16位，挂 APB1，内部时钟 84MHz）
 * 解码模式：正交编码模式 3（TI1 与 TI2 双边沿计数）
 *
 * JGA25-370 编码器规格：
 *   电机轴每转编码器脉冲数 = 11 PPR
 *   正交 x4 解码后        = 44 counts/motor-rev
 *   输出轴每转脉冲数      = 44 × 减速比
 *   （根据实际电机型号修改 ENCODER_GEAR_RATIO）
 *
 * 使用方法：
 *   1. 在外设初始化阶段调用 Encoder_Init()
 *   2. 在 TIM6_DAC_IRQHandler（1kHz）中调用 Encoder_Update()
 *   3. 读取 g_encoder_rpm 作为 PID 反馈量
 */

#ifndef ENCODER_REG_H
#define ENCODER_REG_H

#include "stm32f4xx.h"
#include <stdint.h>

/* ------------------------------------------------------------------ */
/*  电机参数（根据实际型号修改）                                        */
/* ------------------------------------------------------------------ */
/** 电机编码器每转脉冲数（电机轴） */
#define ENCODER_MOTOR_PPR       11U

/** 正交解码倍数（x4 双边沿） */
#define ENCODER_QUADRATURE      4U

/**
 * 减速比（整数）—— 常见型号：34, 50, 75, 100, 150, 210, 298
 * 修改此值以匹配你的 JGA25-370 型号
 */
#define ENCODER_GEAR_RATIO      34U

/** 输出轴每转计数值 = PPR × 4 × 减速比 */
#define ENCODER_COUNTS_PER_REV  (ENCODER_MOTOR_PPR * ENCODER_QUADRATURE * ENCODER_GEAR_RATIO)

/* ------------------------------------------------------------------ */
/*  TIM3 参数                                                          */
/* ------------------------------------------------------------------ */
/** 16 位计数器最大值，用于双向计数时的自动溢出处理 */
#define ENCODER_TIM_ARR         0xFFFFU

/** 计数器中心值（双向计数基准） */
#define ENCODER_CNT_CENTER      0x8000U

/* ------------------------------------------------------------------ */
/*  速度滤波器                                                          */
/* ------------------------------------------------------------------ */
/** 移动平均滤波器窗口大小（必须为 2 的幂，最大 32） */
#define ENCODER_FILTER_SIZE     8U

/* ------------------------------------------------------------------ */
/*  对外暴露的全局变量（在 stm32f4xx_it.c 中直接读取）                 */
/* ------------------------------------------------------------------ */
/** 每次 Encoder_Update() 调用后的计数增量（带符号） */
extern volatile int16_t g_encoder_delta;

/** 输出轴转速，单位 RPM（正值=正转，负值=反转），已滤波 */
extern volatile float   g_encoder_rpm;

/* ------------------------------------------------------------------ */
/*  API                                                                 */
/* ------------------------------------------------------------------ */

/**
 * @brief 初始化 TIM3 为正交编码器模式
 *        配置 PC6/PC7 为 AF2，使能 TIM3 计数器
 *        必须在系统时钟配置（168MHz）之后调用
 */
void Encoder_Init(void);

/**
 * @brief 更新编码器速度测量
 *        在 1kHz 定时中断（TIM6）中调用
 *        内部更新 g_encoder_delta 和 g_encoder_rpm
 */
void Encoder_Update(void);

/**
 * @brief 读取输出轴当前绝对位置（以计数值表示，相对于 Reset 起点）
 * @return 有符号计数值（正=正方向，负=反方向）
 */
int32_t Encoder_GetCount(void);

/**
 * @brief 将位置计数清零（计数器重置到中心值）
 *        同时清除速度滤波器历史
 */
void Encoder_ResetCount(void);

/**
 * @brief 读取最新滤波后的转速
 * @return 转速，单位 RPM
 */
float Encoder_GetSpeed_RPM(void);

#endif /* ENCODER_REG_H */
