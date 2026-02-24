#ifndef SPI_H
#define SPI_H

/*
 * spi.h — SPI主机通信模块
 *
 * STM32作为SPI主机，连接两个从机：
 *   1. FPGA（Spartan-6 AX309）：发送PWM占空比值（0-1000）
 *   2. W25Q64 Flash：存储PID参数
 *
 * 引脚：PA5(SCLK)，PA7(MOSI)，PA6(MISO)
 *       PA4(NSS_FPGA)，PB0(NSS_FLASH)  ← 片选分开
 */

void SPI_Init(void);

/* 向FPGA发送PWM占空比（0 ~ 1000，代表0%~100%） */
void SPI_SendPWMDuty(unsigned short duty);

/* Flash：写入PID参数到指定地址 */
void Flash_WritePIDParams(float kp_speed, float ki_speed, float kp_current, float ki_current);

/* Flash：从Flash读取PID参数 */
void Flash_ReadPIDParams(float *kp_speed, float *ki_speed, float *kp_current, float *ki_current);

#endif /* SPI_H */
