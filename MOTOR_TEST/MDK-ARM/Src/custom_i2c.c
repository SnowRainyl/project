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

/* 使用已有的 SSD1306 驱动库来辅助显示字符串 */
#include <string.h>

/* 底层I2C驱动已经由 OLED_SSD1306 库处理，这里不再需要
   以前的初始化、起停、写字节等代码都已移除 */

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

void I2C_Start(void) {
    I2C1->CR1 |= I2C_CR1_START;
    while (!(I2C1->SR1 & I2C_SR1_SB));
}
void I2C_Stop(void) {
    I2C1->CR1 |= I2C_CR1_STOP;
}
void I2C_SendByte(unsigned char byte) {
    I2C1->DR = byte;
    while (!(I2C1->SR1 & I2C_SR1_TXE));
}

/* helper to write memory (control + data sequence) */
void i2c_write_memory(uint8_t slav_add, uint8_t memadd, uint8_t data, uint8_t length)
{
    uint8_t send_arr[length+1];
    send_arr[0] = memadd;
    send_arr[1] = data; // 注意：如果 length > 1，原逻辑中 send_arr 的后续元素未被初始化
    
    // 1. 启用 I2C 外设
    I2C1->CR1 |= I2C_CR1_PE;

    // 2. 生成 START 起始条件
    I2C1->CR1 |= I2C_CR1_START;
    
    // 等待 START 条件发送完成 (SB = 1)
    while (!(I2C1->SR1 & I2C_SR1_SB));

    // 3. 发送从机地址 (7位地址左移1位，最低位0表示写模式)
    I2C1->DR = (slav_add << 1);
    
    // 等待地址发送完成并匹配 (ADDR = 1)
    while (!(I2C1->SR1 & I2C_SR1_ADDR));

    // 清除 ADDR 标志位：在 F4 中，必须按顺序读取 SR1 然后读取 SR2 才能清除 ADDR
    (void)I2C1->SR1;
    (void)I2C1->SR2;

    // 4. 循环发送数据 (内存地址 + 实际数据)
    for (int i = 0; i < (length + 1); i++)
    {
        // 等待数据寄存器为空 (TXE = 1)
        while (!(I2C1->SR1 & I2C_SR1_TXE));
        
        // 写入数据到 DR 寄存器 (相当于 F3 的 TXDR)
        I2C1->DR = send_arr[i];
    }

    // 5. 等待所有字节传输完成 (BTF = 1)
    // 必须等待最后一个字节不仅进入了移位寄存器，而且已经在总线上发送完毕
    while (!(I2C1->SR1 & I2C_SR1_BTF));

    // 6. 软件手动生成 STOP 停止条件 (因为 F4 没有 AUTOEND)
    I2C1->CR1 |= I2C_CR1_STOP;

    // 禁用 I2C 外设 (根据你的原代码保留，不过通常 I2C 可以在初始化后一直保持 Enable 状态)
    I2C1->CR1 &= ~I2C_CR1_PE;
}


