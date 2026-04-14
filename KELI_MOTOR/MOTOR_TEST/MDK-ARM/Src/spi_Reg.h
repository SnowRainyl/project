#ifndef __SPI_REG_H
#define __SPI_REG_H

#include "stm32f4xx.h"

/* ================== 宏定义：片选引脚控制 ================== */
// 使用 BSRR 寄存器进行高效率的位带操作
// BSRR 低 16 位写 1 拉高，高 16 位写 1 拉低

// SPI1 (NOR Flash) CS 引脚：PA4
#define FLASH_CS_LOW()      (GPIOA->BSRR = (1U << (4 + 16)))
#define FLASH_CS_HIGH()     (GPIOA->BSRR = (1U << 4))

// SPI2 (FPGA) CS 引脚：PB12
#define FPGA_CS_LOW()       (GPIOB->BSRR = (1U << (12 + 16)))
#define FPGA_CS_HIGH()      (GPIOB->BSRR = (1U << 12))

/* ================== 函数声明 ================== */

// 初始化函数
void SPI1_Flash_Init(void);
void SPI2_FPGA_Init(void);

// 底层读写函数
uint8_t SPI1_ReadWriteByte(uint8_t tx_data);
uint8_t SPI2_ReadWriteByte(uint8_t tx_data);

#endif /* __SPI_BSP_H */