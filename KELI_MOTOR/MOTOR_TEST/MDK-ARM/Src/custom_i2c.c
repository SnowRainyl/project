/*
 * i2c.c — I2C + OLED SSD1306显示模块实现
 *
 * 硬件：0.96寸 SSD1306 OLED（4针IIC，工作电压3.3V）
 * 引脚：PB6(SCL)，PB7(SDA)
 * I2C地址：0x3C（模块默认）
 *
 * 显示布局（128×64像素）：
 *   演示用字符串由调用者提供。可通过OLED_ShowString打印。
 */

#include "custom_i2c.h"
#include "stm32f4xx.h"

/* 简单的寄存器级 I2C1 驱动，仅支持写操作，用于 OLED 及其它需要
   直接访问 I2C 的模块。初始化必须先调用一次。 */

void I2C_Init(void) {
    /* 步骤1 - 使能GPIOB、I2C1时钟 */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;   // PB6/7
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;    // I2C1

    /* 步骤2 - PB6/PB7 复用开漏 */
    GPIOB->MODER &= ~(GPIO_MODER_MODE6 | GPIO_MODER_MODE7);
    GPIOB->MODER |= (GPIO_MODER_MODE6_1 | GPIO_MODER_MODE7_1);
    GPIOB->OTYPER |= (GPIO_OTYPER_OT6 | GPIO_OTYPER_OT7);
    GPIOB->OSPEEDR |= (GPIO_OSPEEDR_OSPEED6 | GPIO_OSPEEDR_OSPEED7);
    GPIOB->PUPDR &= ~(GPIO_PUPDR_PUPD6 | GPIO_PUPDR_PUPD7);
    GPIOB->PUPDR |= (GPIO_PUPDR_PUPD6_0 | GPIO_PUPDR_PUPD7_0);
    GPIOB->AFR[0] &= ~((0xF << (6*4)) | (0xF << (7*4)));
    GPIOB->AFR[0] |= ((4 << (6*4)) | (4 << (7*4))); // AF4

    /* 步骤3 - I2C 参数设置 100kHz, TRISE≈37 */
    I2C1->CR2 = 36;
    I2C1->CCR = 180;
    I2C1->TRISE = 37;
    I2C1->CR1 |= I2C_CR1_PE;
}

#define I2C_TIMEOUT  10000U

/* helper to write memory (control + data sequence) */
void i2c_write_memory(uint8_t slav_add, uint8_t memadd, uint8_t data, uint8_t length)
{
    uint8_t send_arr[length+1];
    send_arr[0] = memadd;
    send_arr[1] = data; // 注意：如果 length > 1，原逻辑中 send_arr 的后续元素未被初始化
    
    uint32_t t;

    // 0. 清除上次残留的错误标志（AF/ARLO/BERR），否则后续事务无法启动
    I2C1->SR1 &= ~(I2C_SR1_AF | I2C_SR1_ARLO | I2C_SR1_BERR | I2C_SR1_OVR);

    // 1. 等待总线空闲（BUSY=0）
    t = I2C_TIMEOUT;
    while ((I2C1->SR2 & I2C_SR2_BUSY) && --t);
    if (!t) return;

    // 2. 生成 START
    I2C1->CR1 |= I2C_CR1_START;
    t = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_SB) && --t);
    if (!t) { I2C1->CR1 |= I2C_CR1_STOP; return; }

    // 3. 发送从机地址
    I2C1->DR = (slav_add << 1);
    t = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_ADDR) && --t);
    if (!t) {
        I2C1->SR1 &= ~I2C_SR1_AF;   // 清 AF，否则下次仍失败
        I2C1->CR1 |= I2C_CR1_STOP;
        return;
    }

    // 清除 ADDR 标志（读 SR1 再读 SR2）
    (void)I2C1->SR1;
    (void)I2C1->SR2;

    // 4. 循环发送数据
    for (int i = 0; i < (length + 1); i++)
    {
        t = I2C_TIMEOUT;
        while (!(I2C1->SR1 & I2C_SR1_TXE) && --t);
        if (!t) { I2C1->CR1 |= I2C_CR1_STOP; return; }
        I2C1->DR = send_arr[i];
    }

    // 5. 等待最后字节发送完成
    t = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_BTF) && --t);

    // 6. 生成 STOP（PE 保持使能）
    I2C1->CR1 |= I2C_CR1_STOP;
}


