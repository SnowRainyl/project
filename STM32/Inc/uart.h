#ifndef UART_H
#define UART_H

/*
 * uart.h — UART串口调试模块
 *
 * 功能：通过USART1把调试信息发送到电脑（用USB-TTL CH340连接）
 * 波特率：115200
 * 引脚：PA9(TX) → CH340_RX，PA10(RX) → CH340_TX
 */

void UART_Init(void);

/* 发送单个字符 */
void UART_SendChar(char c);

/* 发送字符串 */
void UART_SendString(const char *str);

/* 发送整数（用于打印数值，如转速、PID误差等） */
void UART_SendInt(int val);

#endif /* UART_H */
