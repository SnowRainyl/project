#include "pid.h"

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
    pid->integral     = 0.0f;
    pid->err_prev     = 0.0f;
}

float PID_Calc(PID_TypeDef *pid, float setpoint, float feedback)
{
    float err = setpoint - feedback;

    pid->integral += err;
    if      (pid->integral >  pid->integral_max) { pid->integral =  pid->integral_max; }
    else if (pid->integral < -pid->integral_max) { pid->integral = -pid->integral_max; }

    float deriv   = pid->kd * (err - pid->err_prev);
    pid->err_prev = err;

    float output = pid->kp * err + pid->ki * pid->integral + deriv;

    /* Conditional anti-windup: if output is saturated AND the error would push
     * it further into saturation, roll back this cycle's integral accumulation. */
    if (output > pid->out_max) {
        output = pid->out_max;
        if (err > 0.0f) { pid->integral -= err; }
    } else if (output < pid->out_min) {
        output = pid->out_min;
        if (err < 0.0f) { pid->integral -= err; }
    }

    return output;
}

void PID_Reset(PID_TypeDef *pid)
{
    pid->integral = 0.0f;
    pid->err_prev = 0.0f;
}
