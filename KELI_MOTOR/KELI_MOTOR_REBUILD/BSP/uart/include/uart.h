#ifndef UART_H
#define UART_H

#include <stdint.h>

/*
 * uart.h — UART串口调试模块
 *
 * 硬件：STM32F407G-DISC1
 * 外设：USART2，PA2(TX) / PA3(RX)，连接到外部 CH340 USB-TTL
 * 波特率：115200，8N1
 * 接线：CH340 TXD -> PA3，CH340 RXD -> PA2，GND 共地，VCC 不接
 */

void UART_Init(void);

/* 发送单个字符 */
void UART_SendChar(char c);

/* 发送字符串 */
void UART_SendString(const char *str);

/* 非阻塞接收一个字节：有数据返回1并写入*c，否则返回0 */
uint8_t UART_RecvChar(char *c);

#endif /* UART_H */
