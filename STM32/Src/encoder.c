/*
 * encoder.c — 编码器测速模块实现
 *
 * 使用TIM2的编码器接口模式（Encoder Interface Mode）
 * TIM2_CH1 (PA0) = A相，TIM2_CH2 (PA1) = B相
 *
 * 测速原理：
 *   每隔 SAMPLE_TIME_MS 毫秒读取一次TIM2计数器
 *   转速(RPM) = (计数差值 / 每圈脉冲数) / 采样时间 * 60 * 1000
 *
 * JGA25-370编码器每转脉冲数：需查看编码器规格书确认（常见值：334PPR或类似）
 */

#include "encoder.h"
#include "stm32f10x.h"

/* 每隔多少毫秒采样一次（在主循环中调用Encoder_Update） */
#define SAMPLE_TIME_MS  10

/* 编码器每转脉冲数（PPR），查JGA25-370规格书确认 */
#define ENCODER_PPR     334

static int last_count   = 0;
static int actual_speed = 0; /* 当前转速，RPM */

void Encoder_Init(void) {
    /* TODO: 步骤1 - 使能GPIOA和TIM2时钟 */

    /* TODO: 步骤2 - 配置PA0、PA1为浮空输入（编码器信号输入） */

    /* TODO: 步骤3 - 配置TIM2为编码器模式
       TIM2->SMCR |= (3<<0);       // SMS = 011：T1和T2都计数（4倍频）
       TIM2->CCMR1 |= (1<<0)|(1<<8); // CC1S=01，CC2S=01：输入捕获映射
       TIM2->ARR = 0xFFFF;          // 最大计数值
       TIM2->CNT = 0;               // 清零计数器
       TIM2->CR1 |= (1<<0);         // 使能定时器
    */
}

void Encoder_Update(void) {
    /* 读取当前计数器值 */
    int current_count = (int)(short)TIM2->CNT; /* 强转为有符号数，支持正反转 */

    /* 计算这段时间的脉冲数 */
    int delta = current_count - last_count;
    last_count = current_count;

    /*
     * 转速计算：
     *   delta 脉冲 / ENCODER_PPR脉冲每转 → 转了多少圈
     *   再除以采样时间（s）得到转/秒，乘60得到RPM
     *
     *   RPM = delta / ENCODER_PPR / (SAMPLE_TIME_MS/1000) * 60
     *       = delta * 60000 / (ENCODER_PPR * SAMPLE_TIME_MS)
     */
    actual_speed = delta * 60000 / (ENCODER_PPR * SAMPLE_TIME_MS);
}

int Encoder_GetSpeedRPM(void) {
    return actual_speed;
}
