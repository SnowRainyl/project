#ifndef MOTOR_H
#define MOTOR_H

/*
 * motor.h — TB6612FNG 电机驱动控制模块
 *
 * TB6612FNG 需要三类信号（缺一不可，电机才会转）：
 *
 *   PWMA  ← FPGA 输出的 PWM（已由 spi.c / FPGA 控制）
 *   AIN1  ← STM32 GPIO 控制方向 A
 *   AIN2  ← STM32 GPIO 控制方向 B
 *   STBY  ← STM32 GPIO 使能（高电平=正常工作，低电平=待机）
 *
 * 真值表（TB6612FNG 数据手册）：
 *   AIN1=1, AIN2=0 → 电机正转（顺时针）
 *   AIN1=0, AIN2=1 → 电机反转（逆时针）
 *   AIN1=0, AIN2=0 → 电机滑行停止（coast）
 *   AIN1=1, AIN2=1 → 电机刹车停止（brake）
 *   STBY=0          → 整个驱动芯片关闭（待机省电）
 *
 * 引脚规划（可根据实际接线修改）：
 *   STBY → PB1（推挽输出，默认高电平）
 *   AIN1 → PB10
 *   AIN2 → PB11
 *   PWMA → 来自FPGA（不由STM32直接控制）
 */

void Motor_Init(void);

/* 电机正转，STBY拉高，设置方向，PWM由FPGA控制 */
void Motor_SetForward(void);

/* 电机反转 */
void Motor_SetReverse(void);

/* 制动停止（AIN1=1, AIN2=1） */
void Motor_Brake(void);

/* 待机关闭（STBY=0，整个驱动芯片掉电） */
void Motor_Standby(void);

#endif /* MOTOR_H */
