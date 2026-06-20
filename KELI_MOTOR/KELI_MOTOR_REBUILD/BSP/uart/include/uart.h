#ifndef UART_H
#define UART_H

#include <stdint.h>

/* USART2 — PA2(TX) / PA3(RX), 115200 8N1, APB1=42MHz
 * Wiring: CH340 TXD->PA3, CH340 RXD->PA2, GND->GND, VCC unconnected */

void    UART_Init(void);
void    UART_SendChar(char c);
void    UART_SendString(const char *str);

/* Non-blocking receive: returns 1 and writes *c if data available, else 0 */
uint8_t UART_RecvChar(char *c);

void    USART2_IRQHandler(void);

#endif /* UART_H */
