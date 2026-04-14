#ifndef __PID_H
#define __PID_H

/* ============================================================================
 * 位置式 PID 控制器
 *
 * 适用场景：MCU 计算 PID，输出 12 位占空比（0~4095）发往 FPGA PWM 模块
 *
 * 使用方法：
 *   1. 定义一个 PID_TypeDef 变量
 *   2. 调用 PID_Init() 设置参数
 *   3. 在定时器中断里以固定周期调用 PID_Calc()
 *   4. 将返回值通过 SPI2 发给 FPGA
 * ============================================================================ */

typedef struct {
    /* ---- 整定参数 ---- */
    float kp;           /* 比例系数 */
    float ki;           /* 积分系数（已含 dt：ki_real * dt） */
    float kd;           /* 微分系数（已含 dt：kd_real / dt） */

    /* ---- 内部状态 ---- */
    float integral;     /* 积分累积量 */
    float err_prev;     /* 上一拍误差（用于微分） */

    /* ---- 输出限幅 ---- */
    float out_min;      /* 最小输出（通常 0） */
    float out_max;      /* 最大输出（12位 PWM 对应 4095） */

    /* ---- 积分限幅（防积分饱和） ---- */
    float integral_max; /* 积分项绝对值上限 */
} PID_TypeDef;

/* ============================================================================
 * 函数声明
 * ============================================================================ */

/**
 * @brief  初始化 PID 参数
 * @param  pid          PID 结构体指针
 * @param  kp           比例系数
 * @param  ki           积分系数（传入 ki_real * dt，dt 为控制周期，单位秒）
 * @param  kd           微分系数（传入 kd_real / dt）
 * @param  out_min      输出下限
 * @param  out_max      输出上限（12bit PWM 传 4095.0f）
 * @param  integral_max 积分限幅（建议设为 out_max 的 30%~50%）
 */
void  PID_Init(PID_TypeDef *pid,
               float kp, float ki, float kd,
               float out_min, float out_max,
               float integral_max);

/**
 * @brief  PID 计算（在固定周期定时器中断里调用）
 * @param  pid       PID 结构体指针
 * @param  setpoint  期望值（目标转速、位置等）
 * @param  feedback  反馈值（实测值；暂无编码器时传 0.0f）
 * @return 计算结果，已限幅在 [out_min, out_max]，可直接转换为 uint16_t 发送
 */
float PID_Calc(PID_TypeDef *pid, float setpoint, float feedback);

/**
 * @brief  复位 PID 内部状态（急停、模式切换时调用）
 */
void  PID_Reset(PID_TypeDef *pid);

#endif /* __PID_H */
