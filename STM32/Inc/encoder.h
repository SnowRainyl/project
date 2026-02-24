#ifndef ENCODER_H
#define ENCODER_H

/*
 * encoder.h — 编码器测速模块
 *
 * 使用TIM2的编码器模式读取JGA25-370电机的编码器信号
 * TIM2_CH1(PA0) = 编码器A相，TIM2_CH2(PA1) = 编码器B相
 *
 * 每隔固定时间（如10ms）读一次计数器差值，换算成RPM
 */

void Encoder_Init(void);

/* 定时调用（建议在定时中断里每10ms调用一次） */
void Encoder_Update(void);

/* 获取当前实际转速（RPM） */
int Encoder_GetSpeedRPM(void);

#endif /* ENCODER_H */
