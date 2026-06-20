#ifndef PID_H
#define PID_H

/* Positional PID with integral clamping and conditional anti-windup */

typedef struct {
    float kp;
    float ki;
    float kd;
    float integral;
    float err_prev;
    float out_min;
    float out_max;
    float integral_max;
} PID_TypeDef;

void  PID_Init(PID_TypeDef *pid,
               float kp, float ki, float kd,
               float out_min, float out_max,
               float integral_max);

float PID_Calc(PID_TypeDef *pid, float setpoint, float feedback);

void  PID_Reset(PID_TypeDef *pid);

#endif /* PID_H */
