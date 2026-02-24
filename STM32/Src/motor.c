/*
 * motor.c — TB6612FNG 电机驱动方向和使能控制
 *
 * 本文件只控制方向和使能，PWM占空比由 FPGA 通过 SPI 接收后独立输出。
 *
 * 硬件接线（STM32最小系统板 → TB6612FNG 模块）：
 *   STM32 PB1  → TB6612 STBY  （使能，高=工作，低=待机）
 *   STM32 PB10 → TB6612 AIN1  （方向控制A）
 *   STM32 PB11 → TB6612 AIN2  （方向控制B）
 *   FPGA  GPIO → TB6612 PWMA  （PWM信号，由FPGA生成）
 *   12V电源    → TB6612 VM    （电机电源）
 *   3.3V电源   → TB6612 VCC   （逻辑电源）
 *   电机两端   → TB6612 AO1, AO2
 *
 * 参考：RM0008 第9章（GPIO），TB6612FNG Datasheet
 */

#include "motor.h"
#include "stm32f10x.h"

/* 操作宏：BRR=拉低，BSRR=拉高 */
#define STBY_HIGH()  (GPIOB->BSRR = (1 << 1))   /* 使能驱动芯片 */
#define STBY_LOW()   (GPIOB->BRR  = (1 << 1))   /* 待机 */
#define AIN1_HIGH()  (GPIOB->BSRR = (1 << 10))
#define AIN1_LOW()   (GPIOB->BRR  = (1 << 10))
#define AIN2_HIGH()  (GPIOB->BSRR = (1 << 11))
#define AIN2_LOW()   (GPIOB->BRR  = (1 << 11))

void Motor_Init(void) {
    /* ── 步骤1：使能GPIOB时钟 ─────────────────────────────────────
     * RCC->APB2ENR bit3 = IOPBEN
     */
    RCC->APB2ENR |= (1 << 3);

    /* ── 步骤2：配置 PB1, PB10, PB11 为推挽输出 50MHz ────────────
     * CRL控制PB0~PB7：PB1在bit[7:4]
     * CRH控制PB8~PB15：PB10在bit[11:8]，PB11在bit[15:12]
     * 推挽输出50MHz：CNF=00, MODE=11 → 0b0011 = 0x3
     */
    /* PB1（STBY）in CRL */
    GPIOB->CRL &= ~(0xFU << 4);
    GPIOB->CRL |=  (0x3U << 4);  /* 推挽输出 50MHz */

    /* PB10（AIN1）in CRH */
    GPIOB->CRH &= ~(0xFU << 8);
    GPIOB->CRH |=  (0x3U << 8);

    /* PB11（AIN2）in CRH */
    GPIOB->CRH &= ~(0xFU << 12);
    GPIOB->CRH |=  (0x3U << 12);

    /* ── 步骤3：初始状态 —— 待机，等待主程序发出方向命令 ──────── */
    STBY_LOW();   /* 先待机，不让电机意外动 */
    AIN1_LOW();
    AIN2_LOW();
}

void Motor_SetForward(void) {
    /* 正转：AIN1=1, AIN2=0 */
    AIN1_HIGH();
    AIN2_LOW();
    STBY_HIGH();  /* 使能驱动芯片，FPGA输出的PWM开始生效 */
}

void Motor_SetReverse(void) {
    /* 反转：AIN1=0, AIN2=1 */
    AIN1_LOW();
    AIN2_HIGH();
    STBY_HIGH();
}

void Motor_Brake(void) {
    /* 刹车：AIN1=1, AIN2=1（短路制动，比滑行停得快） */
    AIN1_HIGH();
    AIN2_HIGH();
    STBY_HIGH();
}

void Motor_Standby(void) {
    /* 关闭整个驱动芯片，降低功耗 */
    STBY_LOW();
}
