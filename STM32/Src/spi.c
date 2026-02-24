/*
 * spi.c — SPI主机通信模块实现
 *
 * STM32作为SPI主机（Master），两个从机用不同片选（NSS）区分：
 *   PA4(NSS_FPGA)  → FPGA Spartan-6
 *   PB0(NSS_FLASH) → W25Q64 Flash
 *
 * SPI1引脚：
 *   PA5 = SCLK，PA7 = MOSI，PA6 = MISO
 *
 * 通信协议（MCU → FPGA）：
 *   发送2字节：高字节 + 低字节，组成16位PWM占空比值（0~1000）
 */

#include "spi.h"
#include "stm32f10x.h"

/* 拉低/拉高片选引脚的宏 */
#define NSS_FPGA_LOW()   /* TODO: GPIOA->BRR  = (1<<4) */
#define NSS_FPGA_HIGH()  /* TODO: GPIOA->BSRR = (1<<4) */
#define NSS_FLASH_LOW()  /* TODO: GPIOB->BRR  = (1<<0) */
#define NSS_FLASH_HIGH() /* TODO: GPIOB->BSRR = (1<<0) */

void SPI_Init(void) {
    /* TODO: 步骤1 - 使能GPIOA、GPIOB、SPI1时钟 */

    /* TODO: 步骤2 - 配置PA5(SCLK)、PA7(MOSI)为复用推挽输出
                     配置PA6(MISO)为浮空输入
                     配置PA4(NSS_FPGA)、PB0(NSS_FLASH)为推挽输出，默认高电平 */

    /* TODO: 步骤3 - 配置SPI1
       SPI1->CR1 = (1<<2)  |  // MSTR: 主机模式
                  (2<<3)  |  // BR[2:0]=010: fPCLK/8（约9MHz，FPGA能接收的速率）
                  (1<<6)  |  // SPE: 使能SPI
                  (1<<8)  |  // SSI
                  (1<<9);    // SSM: 软件管理NSS
    */
}

/* 发送并接收1字节（SPI全双工） */
static unsigned char SPI_Transfer(unsigned char data) {
    /* TODO: while (!(SPI1->SR & (1<<1)));  // 等待TXE
             SPI1->DR = data;
             while (!(SPI1->SR & (1<<0)));  // 等待RXNE
             return SPI1->DR;
    */
    (void)data;
    return 0;
}

void SPI_SendPWMDuty(unsigned short duty) {
    /* duty: 0~1000（0%~100%占空比） */
    NSS_FPGA_LOW();
    SPI_Transfer((unsigned char)(duty >> 8));   /* 高字节先发 */
    SPI_Transfer((unsigned char)(duty & 0xFF)); /* 低字节后发 */
    NSS_FPGA_HIGH();
}

void Flash_WritePIDParams(float kp_speed, float ki_speed, float kp_current, float ki_current) {
    /*
     * TODO: W25Q64写操作流程：
     *   1. 发送写使能命令 (0x06)
     *   2. 发送页编程命令 (0x02) + 24位地址 + 数据
     *   3. 等待写完成（读状态寄存器BUSY位）
     */
    (void)kp_speed; (void)ki_speed; (void)kp_current; (void)ki_current;
}

void Flash_ReadPIDParams(float *kp_speed, float *ki_speed, float *kp_current, float *ki_current) {
    /*
     * TODO: W25Q64读操作流程：
     *   1. 发送读数据命令 (0x03) + 24位地址
     *   2. 连续读出数据字节，重组为float
     */

    /* 如果Flash为空或读取失败，使用默认参数 */
    *kp_speed   = 1.0f;
    *ki_speed   = 0.05f;
    *kp_current = 0.5f;
    *ki_current = 0.1f;
}
