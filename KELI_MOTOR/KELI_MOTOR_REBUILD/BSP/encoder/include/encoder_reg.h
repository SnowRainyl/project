#ifndef ENCODER_REG_H
#define ENCODER_REG_H

#include "stm32f4xx.h"
#include <stdint.h>

/* TIM3 quadrature encoder — PC6=CH1(A), PC7=CH2(B), AF2
 * JGA25-370: 11 PPR motor shaft, x4 decode, gear ratio=34 → 1496 counts/output-rev */

#define ENCODER_MOTOR_PPR       11U
#define ENCODER_QUADRATURE      4U
#define ENCODER_GEAR_RATIO      34U
#define ENCODER_COUNTS_PER_REV  (ENCODER_MOTOR_PPR * ENCODER_QUADRATURE * ENCODER_GEAR_RATIO)

#define ENCODER_TIM_ARR         0xFFFFU
#define ENCODER_CNT_CENTER      0x8000U   /* CNT starts here so signed delta works correctly */
#define ENCODER_FILTER_SIZE     8U
#define ENCODER_UPDATE_HZ       100U      /* called every 10ms from TIM6 ISR */

extern volatile int16_t g_encoder_delta;
extern volatile float   g_encoder_rpm;

void    Encoder_Init(void);
void    Encoder_Update(void);   /* call from 1kHz ISR every 10ms */
int32_t Encoder_GetCount(void);
void    Encoder_ResetCount(void);
float   Encoder_GetSpeed_RPM(void);

#endif /* ENCODER_REG_H */
