#ifndef PID_H
#define PID_H

/*
 * pid.h — 串级PID控制算法
 *
 * 结构：外环（速度环） → 内环（电流环）
 *
 *   速度环：输入=目标转速-实际转速，输出=目标电流
 *   电流环：输入=目标电流-实际电流，输出=PWM占空比（0~1000）
 *
 * 注意：使用增量式PI（不加微分D，避免噪声放大）
 */

/* PID控制器结构体 */
typedef struct {
    float Kp;           /* 比例系数 */
    float Ki;           /* 积分系数 */
    float integral;     /* 积分累积值 */
    float out_max;      /* 输出上限（防止积分饱和） */
    float out_min;      /* 输出下限 */
} PID_t;

/* 初始化PID参数 */
void PID_Init(PID_t *pid, float kp, float ki, float out_max, float out_min);

/* 计算PID输出，error = 目标值 - 实际值 */
float PID_Compute(PID_t *pid, float error);

/*
 * 串级PID总控制函数
 * 输入：目标转速、实际转速、实际电流
 * 返回：最终PWM占空比（0~1000）
 */
int CascadePID_Compute(int target_speed, int actual_speed, int actual_current);

/*
 * 用从Flash读取的参数初始化两个PID控制器
 * 在 main() 启动时调用一次
 */
void PID_InitAll(float kp_speed, float ki_speed, float kp_current, float ki_current);

#endif /* PID_H */
