/*
 * adc.c — ADC采样模块实现
 *
 * 通道1 (PA1)：电位器（经RC低通滤波器） → 目标转速
 * 通道2 (PA2)：INA240放大后的分流电压 → 实际电流
 *
 * RC滤波器（硬件）：R=1kΩ，C=1µF，截止频率约159Hz
 * 软件滑动平均滤波：对最近N次采样取平均
 */

#include "adc.h"
#include "stm32f10x.h"

/* 电机最大转速（JGA25-370，12V时280RPM） */
#define MOTOR_MAX_RPM   280

/* 软件滑动平均滤波窗口大小 */
#define FILTER_SIZE     8

static unsigned int pot_samples[FILTER_SIZE]; /* 电位器采样缓冲 */
static int filter_index = 0;

void ADC_Init(void) {
    /* TODO: 步骤1 - 使能GPIOA和ADC1时钟 */

    /* TODO: 步骤2 - 配置PA1、PA2为模拟输入模式（CNF=00, MODE=00） */

    /* TODO: 步骤3 - 配置ADC1
       - 独立模式
       - 右对齐
       - 采样时间：55.5周期（精度与速度的平衡）
    */

    /* TODO: 步骤4 - 使能ADC，执行校准
       ADC1->CR2 |= (1<<0);   // ADON
       // 等待稳定后执行校准
       ADC1->CR2 |= (1<<2);   // CAL
       while (ADC1->CR2 & (1<<2));
    */
}

/* 读取指定通道的原始ADC值（12位，0~4095） */
static unsigned int ADC_ReadRaw(int channel) {
    /* TODO: 设置通道号，启动单次转换，等待完成，返回ADC1->DR */
    (void)channel;
    return 0; /* 占位，实现后删除 */
}

int ADC_ReadTargetSpeed(void) {
    unsigned int raw = ADC_ReadRaw(1); /* 通道1 = 电位器 */

    /* 存入滑动平均缓冲 */
    pot_samples[filter_index] = raw;
    filter_index = (filter_index + 1) % FILTER_SIZE;

    /* 计算平均值 */
    unsigned int sum = 0;
    for (int i = 0; i < FILTER_SIZE; i++) {
        sum += pot_samples[i];
    }
    unsigned int avg = sum / FILTER_SIZE;

    /* 线性映射：0~4095 → 0~MOTOR_MAX_RPM */
    return (int)((long)avg * MOTOR_MAX_RPM / 4095);
}

int ADC_ReadMotorCurrent(void) {
    unsigned int raw = ADC_ReadRaw(2); /* 通道2 = INA240输出 */

    /*
     * INA240放大器增益（由型号决定）：
     *   A1 = 20 V/V，A2 = 50 V/V，A3 = 100 V/V
     *
     * 电流换算公式：
     *   V_adc = raw * 3.3 / 4095          (ADC输出电压，单位V)
     *   V_shunt = V_adc / 增益             (分流电阻两端电压)
     *   I = V_shunt / R_shunt             (实际电流，单位A)
     *
     * TODO: 根据实际使用的INA240型号和分流电阻值填写增益和R_shunt
     */
    float v_adc    = raw * 3.3f / 4095.0f;
    float gain     = 20.0f;        /* INA240A1的增益 */
    float r_shunt  = 0.1f;        /* 分流电阻：0.1Ω（待购买） */
    float current_A = (v_adc / gain) / r_shunt;

    return (int)(current_A * 1000); /* 返回单位mA */
}
