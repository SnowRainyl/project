/**
 * @file    encoder_reg.c
 * @brief   JGA25-370 编码器驱动实现（寄存器级，STM32F407VG）
 *
 * ─── 硬件连接 ────────────────────────────────────────────────────────
 *
 *   JGA25-370 编码器线序（常见 6 线版本）：
 *     红线  电机驱动器out1
 *     黑线  GND  → GND
 *     huang线  C1   → PC6  (TIM3_CH1)   编码器 A 相
 *     lv 线  C2   → PC7  (TIM3_CH2)   编码器 B 相
 *     蓝线 vcc
 *     bai线  Motor- → 电机驱动器 OUT2
 *
 *   若实际转速方向与期望相反，交换 C1/C2 连接，或修改下方
 *   CCER 寄存器中 CC1P/CC2P 位实现软件方向翻转。
 *
 * ─── 定时器资源分配 ──────────────────────────────────────────────────
 *
 *   TIM6   已占用 → 1kHz 控制环（PID + ADC + SPI2）
 *   TIM3   本模块 → 正交编码器计数（16 位，APB1 总线 84MHz 内部时钟）
 *
 * ─── 正交编码器模式 3（Encoder Mode 3）原理 ─────────────────────────
 *
 *   TIM3 在编码器模式下，CNT 寄存器由编码器脉冲驱动递增/递减，
 *   不需要额外中断，CPU 开销几乎为零。
 *
 *   模式 3：TI1 和 TI2 的上升沿与下降沿均触发计数（4倍频）：
 *     - 正转：CNT 递增
 *     - 反转：CNT 递减（16 位自动溢出回绕）
 *
 * ─── 速度计算 ────────────────────────────────────────────────────────
 *
 *   Encoder_Update() 在 1kHz ISR 中每 1ms 调用一次：
 *     delta  = (int16_t)(CNT_now - CNT_last)   ← 有符号 16 位减法
 *                                                  自动处理溢出回绕
 *     RPM    = delta × 60000 / ENCODER_COUNTS_PER_REV
 *
 *   对 RPM 使用 8 点移动平均滤波，平滑高频噪声。
 */

#include "encoder_reg.h"

/* ------------------------------------------------------------------ */
/*  全局变量                                                            */
/* ------------------------------------------------------------------ */
volatile int16_t g_encoder_delta = 0;
volatile float   g_encoder_rpm   = 0.0f;

/* ------------------------------------------------------------------ */
/*  模块内部静态变量                                                    */
/* ------------------------------------------------------------------ */
static uint16_t s_last_cnt = ENCODER_CNT_CENTER;

/* 移动平均滤波器（8 点） */
static float    s_rpm_buf[ENCODER_FILTER_SIZE];
static uint8_t  s_rpm_idx = 0U;
static float    s_rpm_sum = 0.0f;

/* ------------------------------------------------------------------ */
/*  私有辅助函数                                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief 将一个新 RPM 样本加入移动平均滤波器
 * @param raw_rpm 本次采样的原始 RPM（可正可负）
 * @return 滤波后的平均 RPM
 */
static float RPM_Filter(float raw_rpm)
{
    s_rpm_sum -= s_rpm_buf[s_rpm_idx];      /* 移除最旧的样本 */
    s_rpm_buf[s_rpm_idx] = raw_rpm;
    s_rpm_sum += raw_rpm;                   /* 加入最新的样本 */
    s_rpm_idx = (s_rpm_idx + 1U) % ENCODER_FILTER_SIZE;
    return s_rpm_sum / (float)ENCODER_FILTER_SIZE;
}

/* ------------------------------------------------------------------ */
/*  Encoder_Init                                                        */
/* ------------------------------------------------------------------ */
void Encoder_Init(void)
{
    uint32_t i;

    /* ── 1. 使能时钟 ────────────────────────────────────────────── */

    /* GPIOC 挂 AHB1 总线 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;

    /* TIM3 挂 APB1 总线 */
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;

    /* 时钟使能后至少等 1~2 个 AHB 周期再访问寄存器（读回确保时序） */
    (void)RCC->APB1ENR;

    /* ── 2. 配置 PC6（TIM3_CH1）和 PC7（TIM3_CH2）─────────────── */

    /*
     * MODER：10 = 复用功能模式
     *   PC6 → bits [13:12]
     *   PC7 → bits [15:14]
     */
    GPIOC->MODER &= ~((3UL << (6U * 2U)) | (3UL << (7U * 2U)));
    GPIOC->MODER |=  ((2UL << (6U * 2U)) | (2UL << (7U * 2U)));

    /*
     * OSPEEDR：11 = 高速（编码器脉冲上升时间短，需要高速 IO）
     *   PC6 → bits [13:12]
     *   PC7 → bits [15:14]
     */
    GPIOC->OSPEEDR |= ((3UL << (6U * 2U)) | (3UL << (7U * 2U)));

    /*
     * PUPDR：01 = 上拉（提高抗干扰能力；若编码器输出为推挽可改为 00）
     *   PC6 → bits [13:12]
     *   PC7 → bits [15:14]
     */
    GPIOC->PUPDR &= ~((3UL << (6U * 2U)) | (3UL << (7U * 2U)));
    GPIOC->PUPDR |=  ((1UL << (6U * 2U)) | (1UL << (7U * 2U)));

    /*
     * AFR[0]（AFRL）：AF2 = TIM3
     *   PC6 → bits [27:24]（AFRL 第 6 组，每组 4 位）
     *   PC7 → bits [31:28]（AFRL 第 7 组）
     */
    GPIOC->AFR[0] &= ~((0xFUL << (6U * 4U)) | (0xFUL << (7U * 4U)));
    GPIOC->AFR[0] |=  ((0x2UL << (6U * 4U)) | (0x2UL << (7U * 4U)));

    /* ── 3. 配置 TIM3 为正交编码器模式 ─────────────────────────── */

    /* 预分频 = 0（直接由编码器脉冲驱动计数，不分频） */
    TIM3->PSC = 0U;

    /* 自动重装载值 = 0xFFFF（16 位满量程，允许自由回绕） */
    TIM3->ARR = ENCODER_TIM_ARR;

    /*
     * CCMR1（捕获/比较模式寄存器 1）：
     *   CC1S[1:0] = 01：IC1 映射到 TI1（编码器 A 相）
     *   IC1F[3:0] = 0001：数字滤波器，f_sampling=f_DTS，N=2
     *                      滤除宽度 < 2 个采样周期的毛刺
     *   CC2S[1:0] = 01：IC2 映射到 TI2（编码器 B 相）
     *   IC2F[3:0] = 0001：同上
     *
     *   寄存器布局：
     *     bit [1:0]   CC1S
     *     bit [3:2]   IC1PSC（分频，设为 00 不分频）
     *     bit [7:4]   IC1F
     *     bit [9:8]   CC2S
     *     bit [11:10] IC2PSC
     *     bit [15:12] IC2F
     */
    TIM3->CCMR1 = (1UL << 0U)    /* CC1S = 01 */
                | (1UL << 4U)    /* IC1F = 0001（2 点滤波） */
                | (1UL << 8U)    /* CC2S = 01 */
                | (1UL << 12U);  /* IC2F = 0001（2 点滤波） */

    /*
     * CCER（捕获/比较使能寄存器）：
     *   CC1P = 0：TI1 上升沿有效（不反向）
     *   CC1NP = 0：与 CC1P 配合，选择极性
     *   CC2P = 0：TI2 上升沿有效
     *   CC2NP = 0
     *
     *   若电机实际转速方向相反，将 CC1P 或 CC2P 其中一位置 1 即可翻转。
     *   也可交换编码器 A/B 相接线。
     */
    TIM3->CCER = 0U;

    /*
     * SMCR（从模式控制寄存器）：
     *   SMS[2:0] = 011：编码器模式 3
     *     → TI1 和 TI2 的边沿均触发计数（正交 x4 解码）
     *     → 上升沿时根据另一相电平决定方向
     */
    TIM3->SMCR = (3UL << 0U);  /* SMS = 011 */

    /* ── 4. 初始化计数器到中心值（便于有符号 int16_t 差值运算） ── */
    TIM3->CNT = ENCODER_CNT_CENTER;
    s_last_cnt = ENCODER_CNT_CENTER;

    /* 清空速度滤波器 */
    s_rpm_idx = 0U;
    s_rpm_sum = 0.0f;
    for (i = 0U; i < ENCODER_FILTER_SIZE; i++)
    {
        s_rpm_buf[i] = 0.0f;
    }

    /* ── 5. 启动定时器 ──────────────────────────────────────────── */
    /* CR1：CEN = 1，启动计数器 */
    TIM3->CR1 |= TIM_CR1_CEN;
}

/* ------------------------------------------------------------------ */
/*  Encoder_Update — 在 TIM6 1kHz ISR 中调用                          */
/* ------------------------------------------------------------------ */
void Encoder_Update(void)
{
    uint16_t current_cnt;
    float raw_rpm;

    current_cnt = (uint16_t)(TIM3->CNT);

    /*
     * 有符号 16 位减法：自动处理 16 位溢出回绕。
     *
     * 示例（正转到达溢出边界）：
     *   last=0xFFF0, now=0x0010 → (int16_t)(0x0010 - 0xFFF0) = +32 ✓
     *
     * 示例（反转到达溢出边界）：
     *   last=0x0010, now=0xFFF0 → (int16_t)(0xFFF0 - 0x0010) = -32 ✓
     */
    g_encoder_delta = (int16_t)(current_cnt - s_last_cnt);
    s_last_cnt = current_cnt;

    /*
     * 速度计算（调用周期 = 1ms）：
     *
     *   delta [counts/ms]
     *   × 60000 [ms/min]           ← 将 1/ms 换算到 1/min
     *   ÷ ENCODER_COUNTS_PER_REV   ← 将 counts 换算到转
     *   = RPM（输出轴）
     */
    raw_rpm = (float)g_encoder_delta
            * 60000.0f
            / (float)ENCODER_COUNTS_PER_REV;

    g_encoder_rpm = RPM_Filter(raw_rpm);
}

/* ------------------------------------------------------------------ */
/*  其余 API                                                            */
/* ------------------------------------------------------------------ */

int32_t Encoder_GetCount(void)
{
    /*
     * 返回相对于初始（或上次 Reset）位置的有符号计数值。
     * TIM3->CNT 以 ENCODER_CNT_CENTER 为零点偏移。
     */
    return (int32_t)(int16_t)((uint16_t)(TIM3->CNT) - ENCODER_CNT_CENTER);
}

void Encoder_ResetCount(void)
{
    uint32_t i;

    TIM3->CNT   = ENCODER_CNT_CENTER;
    s_last_cnt  = ENCODER_CNT_CENTER;

    g_encoder_delta = 0;
    g_encoder_rpm   = 0.0f;

    s_rpm_idx = 0U;
    s_rpm_sum = 0.0f;
    for (i = 0U; i < ENCODER_FILTER_SIZE; i++)
    {
        s_rpm_buf[i] = 0.0f;
    }
}

float Encoder_GetSpeed_RPM(void)
{
    return g_encoder_rpm;
}
