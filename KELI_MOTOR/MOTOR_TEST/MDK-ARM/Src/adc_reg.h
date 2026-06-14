#ifndef __ADC_REG_H
#define __ADC_REG_H

#include "stm32f4xx.h"

/* =============================================================================
 * ADC1 配置说明
 *
 * 通道 0：PA0 → ADC1_IN0，电位器（目标转速给定）
 * 通道 1：PA1 → ADC1_IN1，INA240 电流检测放大器输出（实际电流反馈）
 *
 * 时钟  : APB2(84MHz) / 4 = 21MHz
 * 分辨率: 12 位，0~4095
 * 转换  : 软件触发单次转换，动态切换通道，轮询 EOC
 * ============================================================================= */

/* ------------------------------------------------------------------ */
/*  INA240 电流采样硬件参数（根据实际电路修改）                        */
/* ------------------------------------------------------------------ */
/** INA240 增益：A1=20V/V, A2=50V/V, A3=100V/V */
#define ADC_CURR_INA240_GAIN     50.0f  /* INA240A2 增益 50V/V */

/** 采样电阻阻值（毫欧，mΩ）。例：0.1Ω = 100mΩ */
#define ADC_CURR_SHUNT_MOHM      100.0f

/**
 * INA240 REF 引脚电压（mV）：
 *   单向电流（REF 接 GND）        → 0.0f
 *   双向电流（REF 接 VCC/2=1.65V）→ 1650.0f
 */
#define ADC_CURR_VREF_MV         1650.0f  /* 自动校准失败时使用的后备零点 */

/** ADC 参考电压（mV），等于 MCU VDD = 3300mV */
#define ADC_VCC_MV               3300.0f

/* ------------------------------------------------------------------ */
/*  电流滤波器窗口（8 点移动平均）                                     */
/* ------------------------------------------------------------------ */
#define ADC_CURR_FILTER_SIZE     8U
#define ADC_CURR_ZERO_SAMPLES    64U
#define ADC_CURR_BURST_SAMPLES   192U

/* ------------------------------------------------------------------ */
/*  对外暴露的全局变量                                                  */
/* ------------------------------------------------------------------ */
/** 最新滤波后的电流值（mA），供 main.c 显示和外部读取 */
extern volatile float g_motor_current_mA;
extern volatile uint16_t g_motor_current_raw;

/* ------------------------------------------------------------------ */
/*  API                                                                 */
/* ------------------------------------------------------------------ */

/** 初始化 ADC1（PA0 电位器通道 + PA1 电流通道，公共寄存器配置） */
void     ADC1_Init(void);

/** 电机确认停机后重新采集 INA240 零电流偏置 */
void     ADC1_CalibrateCurrentZero(void);

/** 读取指定通道的原始 12 位值（0~4095），ch 传 0 或 1 */
uint16_t ADC1_ReadChannel(uint8_t ch);

/** 电位器：返回 16 点均值滤波后的原始值（0~4095） */
uint16_t ADC1_Read_Filtered(void);

/** 电流：单次采样并换算为毫安（mA） */
float    ADC1_ReadCurrent_mA(void);

/** 电流：8 点移动平均滤波后的毫安值，同时更新 g_motor_current_mA */
float    ADC1_ReadCurrent_Filtered_mA(void);

/* 保留旧接口名称（兼容已有调用） */
static inline uint16_t ADC1_Read(void) { return ADC1_ReadChannel(0U); }

#endif /* __ADC_REG_H */
