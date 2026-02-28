#ifndef UART_H
#define UART_H

/*
 * uart.h — UART串口调试模块
 *
 * 硬件：STM32F407G-DISC1
 * 外设：USART2，PA2(TX) / PA3(RX)，连接到板载ST-Link虚拟串口
 * 波特率：115200，8N1
 * 接线：只需一根 Mini-USB 线接ST-Link口，无需CH340
 */

void UART_Init(void);

/* 发送单个字符 */
void UART_SendChar(char c);

/* 发送字符串 */
void UART_SendString(const char *str);

/* 发送整数（用于打印数值，如转速、PID误差等） */
void UART_SendInt(int val);

#endif /* UART_H */
