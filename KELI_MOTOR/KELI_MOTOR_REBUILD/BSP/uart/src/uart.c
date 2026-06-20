#include "uart.h"
#include "stm32f4xx.h"

#define UART_RX_BUFFER_SIZE  64U
#define UART_RX_BUFFER_MASK  (UART_RX_BUFFER_SIZE - 1U)

static volatile uint8_t s_rx_buf[UART_RX_BUFFER_SIZE];
static volatile uint8_t s_rx_head;
static volatile uint8_t s_rx_tail;

void UART_Init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /* PA2=TX, PA3=RX — AF7 (USART2) */
    GPIOA->MODER  &= ~((3U << (2U * 2U)) | (3U << (3U * 2U)));
    GPIOA->MODER  |=  ((2U << (2U * 2U)) | (2U << (3U * 2U)));
    GPIOA->AFR[0] &= ~((0xFU << (2U * 4U)) | (0xFU << (3U * 4U)));
    GPIOA->AFR[0] |=  ((7U   << (2U * 4U)) | (7U   << (3U * 4U)));

    /* APB1=42MHz, BRR=0x16D → 115200 baud */
    USART2->BRR = 0x16DU;

    s_rx_head = 0U;
    s_rx_tail = 0U;
    USART2->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE | USART_CR1_UE;
    NVIC_SetPriority(USART2_IRQn, 1U);
    NVIC_EnableIRQ(USART2_IRQn);
}

void UART_SendChar(char c)
{
    while (!(USART2->SR & USART_SR_TXE)) {}
    USART2->DR = (unsigned char)c;
}

void UART_SendString(const char *str)
{
    while (*str) {
        UART_SendChar(*str++);
    }
}

uint8_t UART_RecvChar(char *c)
{
    uint8_t tail = s_rx_tail;
    if (tail == s_rx_head) {
        return 0U;
    }
    *c = (char)s_rx_buf[tail];
    s_rx_tail = (uint8_t)((tail + 1U) & UART_RX_BUFFER_MASK);
    return 1U;
}

void USART2_IRQHandler(void)
{
    uint32_t sr = USART2->SR;

    if ((sr & (USART_SR_RXNE | USART_SR_ORE | USART_SR_NE |
               USART_SR_FE  | USART_SR_PE)) != 0U) {
        uint8_t data = (uint8_t)USART2->DR;   /* reading DR clears error flags */

        if ((sr & USART_SR_RXNE) != 0U) {
            uint8_t next = (uint8_t)((s_rx_head + 1U) & UART_RX_BUFFER_MASK);
            if (next != s_rx_tail) {
                s_rx_buf[s_rx_head] = data;
                s_rx_head = next;
            }
        }
    }
}
