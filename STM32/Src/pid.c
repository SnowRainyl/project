/*
 * pid.c — 串级PID控制算法实现
 *
 * 结构：
 *
 *   目标转速 ─→ [速度环PI] ─→ 目标电流 ─→ [电流环PI] ─→ PWM占空比
 *       ↑                          ↑
 *   实际转速                    实际电流
 *
 * 使用PI（不加D，因为D对噪声敏感）
 * 输出限幅防止积分饱和
 *
 * 初始PID参数需要通过实验调整：
 *   1. 先把Ki设为0，调Kp直到响应合适（稍有振荡）
 *   2. 再慢慢加Ki消除稳态误差
 *   3. 先调外环（速度），再调内环（电流）
 */

#include "pid.h"

/* 速度环和电流环各自的PID控制器实例 */
static PID_t speed_pid;
static PID_t current_pid;

void PID_Init(PID_t *pid, float kp, float ki, float out_max, float out_min) {
    pid->Kp       = kp;
    pid->Ki       = ki;
    pid->integral = 0.0f;
    pid->out_max  = out_max;
    pid->out_min  = out_min;
}

float PID_Compute(PID_t *pid, float error) {
    /* 积分累加 */
    pid->integral += error;

    /* 积分限幅（防止积分饱和） */
    if (pid->integral > pid->out_max) pid->integral = pid->out_max;
    if (pid->integral < pid->out_min) pid->integral = pid->out_min;

    /* PI输出 */
    float output = pid->Kp * error + pid->Ki * pid->integral;

    /* 输出限幅 */
    if (output > pid->out_max) output = pid->out_max;
    if (output < pid->out_min) output = pid->out_min;

    return output;
}

int CascadePID_Compute(int target_speed, int actual_speed, int actual_current) {
    /* === 外环：速度环 === */
    float speed_error = (float)(target_speed - actual_speed); /* 单位：RPM */

    /*
     * 速度环输出 = 目标电流（mA）
     * 初始参数（需调试）：Kp=1.0, Ki=0.05
     * 输出限幅：0 ~ 2000 mA（根据电机额定电流设置上限）
     */
    float target_current = PID_Compute(&speed_pid, speed_error);

    /* === 内环：电流环 === */
    float current_error = target_current - (float)actual_current; /* 单位：mA */

    /*
     * 电流环输出 = PWM占空比（0~1000，对应0%~100%）
     * 初始参数（需调试）：Kp=0.5, Ki=0.1
     */
    float pwm = PID_Compute(&current_pid, current_error);

    return (int)pwm;
}

/* 在main.c启动时调用，用Flash里存的参数初始化PID */
void PID_InitAll(float kp_speed, float ki_speed, float kp_current, float ki_current) {
    PID_Init(&speed_pid,   kp_speed,   ki_speed,   2000.0f,  0.0f);
    PID_Init(&current_pid, kp_current, ki_current, 1000.0f,  0.0f);
}
