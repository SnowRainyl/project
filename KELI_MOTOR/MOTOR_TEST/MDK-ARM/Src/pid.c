#include "pid.h"

/* ============================================================================
 * 位置式 PID 实现（带积分限幅 + 输出限幅）
 *
 * 位置式 PID 公式：
 *   output = kp*e + ki*∑e + kd*(e - e_prev)
 *
 * 其中 ki 和 kd 在 PID_Init 时已折算进 dt，调用方直接传 setpoint/feedback 即可。
 * ============================================================================ */

void PID_Init(PID_TypeDef *pid,
              float kp, float ki, float kd,
              float out_min, float out_max,
              float integral_max)
{
    pid->kp           = kp;
    pid->ki           = ki;
    pid->kd           = kd;
    pid->out_min      = out_min;
    pid->out_max      = out_max;
    pid->integral_max = integral_max;

    pid->integral  = 0.0f;
    pid->err_prev  = 0.0f;
}

float PID_Calc(PID_TypeDef *pid, float setpoint, float feedback)
{
    float err = setpoint - feedback;

    /* ---- 积分累加 ---- */
    pid->integral += err;
    if      (pid->integral >  pid->integral_max) pid->integral =  pid->integral_max;
    else if (pid->integral < -pid->integral_max) pid->integral = -pid->integral_max;

    /* ---- 微分 ---- */
    float deriv = pid->kd * (err - pid->err_prev);
    pid->err_prev = err;

    /* ---- PID 求和 ---- */
    float output = pid->kp * err + pid->ki * pid->integral + deriv;

    /* ---- 输出限幅 + 条件积分回退（Anti-windup）----
     *
     * 问题根源：输出饱和时积分继续累积，电机超调后 integral 太大，
     * 减速太慢，导致转速长时间高于 setpoint。
     *
     * 解决方案：若输出越界 AND 当前误差方向会让越界更严重，
     * 则撤销本次 integral += err，使积分停在恰好刚刚饱和的值。
     *   - 上饱和（output > max）且 err > 0：不再累积正向积分
     *   - 下饱和（output < min）且 err < 0：不再累积负向积分
     */
    if (output > pid->out_max) {
        output = pid->out_max;
        if (err > 0.0f) pid->integral -= err;   /* 回退本次累加 */
    } else if (output < pid->out_min) {
        output = pid->out_min;
        if (err < 0.0f) pid->integral -= err;   /* 回退本次累加 */
    }

    return output;
}

void PID_Reset(PID_TypeDef *pid)
{
    pid->integral = 0.0f;
    pid->err_prev = 0.0f;
}
