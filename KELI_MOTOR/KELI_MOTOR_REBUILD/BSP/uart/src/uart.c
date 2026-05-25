/*
 * uart.c - USART2 寄存器级练习骨架
 *
 * 这一版不是完整答案，而是给你自己查手册、自己填寄存器用的。
 *
 * 硬件连接：
 *   CH340 TXD -> PA3  USART2_RX
 *   CH340 RXD -> PA2  USART2_TX
 *   CH340 GND -> STM32 GND
 *   CH340 VCC 不接
 *
 * 串口助手设置：
 *   115200, 8N1
 *
 * 当前时钟前提：
 *   SystemClock_Config() 已经把 APB1 配成 42 MHz
 *   USART2 挂在 APB1 上，所以 USART2 的输入时钟是 42 MHz
 *
 * 手册使用顺序建议：
 *   1. 先看 datasheet，确认 PA2/PA3 的复用功能编号是 AF7
 *   2. 再看 RM0090 的 RCC 章节，确认怎么打开 GPIOA/USART2 时钟
 *   3. 再看 RM0090 的 GPIO 章节，确认 MODER/AFR 怎么配
 *   4. 最后看 RM0090 的 USART 章节，确认 BRR/CR1/SR/DR 怎么用
 */

#include "uart.h"
#include "stm32f4xx.h"

#define UART2_BAUDRATE 115200U
#define UART2_PCLK_HZ  42000000U

/*
 * 函数：计算 USART2->BRR
 *
 * 目的：
 *   把 APB1 时钟 42 MHz 转成串口波特率 115200。
 *
 * 去哪里查：
 *   RM0090 -> USART 章节 -> Baud rate generation
 *   RM0090 -> USART_BRR register
 *
 * 你要找什么：
 *   1. OVER8 = 0 时，USARTDIV 怎么算
 *   2. BRR 高位 mantissa 怎么填
 *   3. BRR 低位 fraction 怎么填
 *
 * 目标结果：
 *   APB1 = 42 MHz, baud = 115200 时，BRR 应该约等于 0x16D。
 *
 * 你要做：
 *   把下面 return 0U 改成真正的计算结果。
 *   初学阶段也可以先直接 return 0x16D，然后再补公式。
 */
static uint32_t UART_BRR_Over16(uint32_t pclk_hz, uint32_t baudrate)
{
    return (pclk_hz + (baudrate / 2U)) / baudrate;
}

void UART_Init(void)
{
    /*
     * 第 1 步：打开时钟
     *
     * 为什么要做：
     *   STM32 外设默认很多是没给时钟的。
     *   如果 GPIOA 没开时钟，PA2/PA3 配置不会生效。
     *   如果 USART2 没开时钟，USART2 寄存器配置了也不能工作。
     *
     * 去哪里查：
     *   RM0090 -> RCC 章节
     *
     * 你要找什么：
     *   1. GPIOAEN 在 RCC_AHB1ENR 的第几位
     *   2. USART2EN 在 RCC_APB1ENR 的第几位
     *
     * 你要填写：
     *   RCC->AHB1ENR |= ...
     *   RCC->APB1ENR |= ...
     */
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

    /*
     * 第 2 步：把 PA2/PA3 配成 USART2 功能
     *
     * 为什么要做：
     *   PA2/PA3 默认只是普通 GPIO。
     *   要让它们连接到 USART2 外设内部，必须设置成 Alternate Function。
     *
     * PA2/PA3 在本项目里的作用：
     *   PA2 = USART2_TX -> 接 CH340 RXD
     *   PA3 = USART2_RX -> 接 CH340 TXD
     *
     * 去哪里查：
     *   datasheet -> Alternate function mapping
     *     查 PA2/PA3 对应 USART2 是 AF 几
     *
     *   RM0090 -> GPIO 章节
     *     查 GPIOx_MODER
     *     查 GPIOx_AFRL
     *
     * 你要找什么：
     *   1. MODER 里 00/01/10/11 分别代表什么模式
     *   2. PA2 的 MODER 位在哪里
     *   3. PA3 的 MODER 位在哪里
     *   4. AFRL 每个引脚占几位
     *   5. PA2/PA3 要写入哪个 AF 编号
     *
     * 你要填写：
     *   1. 清除 PA2/PA3 的 MODER 位
     *   2. 设置 PA2/PA3 为复用功能模式
     *   3. 清除 PA2/PA3 的 AFRL 位
     *   4. 设置 PA2/PA3 为 USART2 对应 AF
     */
    GPIOA->MODER &= ~((3U << (2U * 2U)) | (3U << (3U * 2U)));
    GPIOA->MODER |=  ((2U << (2U * 2U)) | (2U << (3U * 2U)));

    GPIOA->AFR[0] &= ~((0xFU << (2U * 4U)) | (0xFU << (3U * 4U)));
    GPIOA->AFR[0] |=  ((7U   << (2U * 4U)) | (7U   << (3U * 4U)));

    /*
     * 第 3 步：配置 USART2 的基本格式
     *
     * 目标格式：
     *   115200, 8N1
     *
     * 8N1 的意思：
     *   8 = 8 个数据位
     *   N = no parity，无校验
     *   1 = 1 个停止位
     *
     * 去哪里查：
     *   RM0090 -> USART 章节
     *
     * 你要看哪些寄存器：
     *   USART_CR1
     *   USART_CR2
     *   USART_CR3
     *   USART_BRR
     *
     * 你要找什么：
     *   1. CR1 里 M 位控制几个数据位
     *   2. CR1 里 PCE 位控制是否开启校验
     *   3. CR1 里 OVER8 位控制过采样 8/16
     *   4. CR2 里 STOP 位控制停止位数量
     *   5. BRR 如何设置波特率
     *
     * 提示：
     *   很多格式位复位默认就是 8N1、过采样 16。
     *   但你要自己查 reset value，确认默认值是不是符合目标。
     */
    USART2->CR1 &= ~USART_CR1_UE;
    USART2->CR1 &= ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_OVER8);
    USART2->CR2 &= ~USART_CR2_STOP;
    USART2->CR3 = 0U;
    USART2->BRR = UART_BRR_Over16(UART2_PCLK_HZ, UART2_BAUDRATE);

    /*
     * 第 4 步：使能 USART2
     *
     * 为什么最后做：
     *   先把格式、波特率、引脚都配好，再打开 USART。
     *
     * 去哪里查：
     *   RM0090 -> USART_CR1 register
     *
     * 你要找什么：
     *   1. TE 是哪一位：发送使能
     *   2. RE 是哪一位：接收使能
     *   3. UE 是哪一位：USART 总使能
     *
     * 你要填写：
     *   USART2->CR1 |= ...
     */
    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;
}

void UART_SendChar(char c)
{
    /*
     * 函数目标：
     *   发送 1 个字符。
     *
     * 发送流程：
     *   1. 等待发送数据寄存器空
     *   2. 把字符写入数据寄存器
     *
     * 去哪里查：
     *   RM0090 -> USART 章节
     *
     * 你要看哪些寄存器：
     *   USART_SR
     *   USART_DR
     *
     * 你要找什么：
     *   1. TXE 是 SR 的第几位
     *   2. TXE = 1 代表什么
     *   3. DR 写入低 8 位是否就是发送一个字节
     *
     * 你要填写：
     *   while 等待 TXE
     *   USART2->DR = ...
     */
    while ((USART2->SR & USART_SR_TXE) == 0U) {
    }
    USART2->DR = (uint8_t)c;
}

void UART_SendString(const char *str)
{
    /*
     * 这个函数不用查寄存器。
     * 它只是不断调用 UART_SendChar()。
     */
    while (*str) {
        UART_SendChar(*str++);
    }
}

uint8_t UART_RecvChar(char *c)
{
    /*
     * 函数目标：
     *   非阻塞接收 1 个字符。
     *
     * 非阻塞的意思：
     *   有数据：读出来，返回 1
     *   没数据：马上返回 0，不在这里死等
     *
     * 去哪里查：
     *   RM0090 -> USART 章节
     *
     * 你要看哪些寄存器：
     *   USART_SR
     *   USART_DR
     *
     * 你要找什么：
     *   1. RXNE 是 SR 的第几位
     *   2. RXNE = 1 代表什么
     *   3. ORE 是什么错误
     *   4. ORE 怎么清除
     *
     * 你要填写：
     *   1. 可选：处理 ORE
     *   2. 如果 RXNE = 1，读取 DR 到 *c，返回 1
     *   3. 如果 RXNE = 0，返回 0
     */
    uint32_t sr = USART2->SR;

    if ((sr & USART_SR_ORE) != 0U) {
        (void)USART2->DR;
        return 0U;
    }

    if ((sr & USART_SR_RXNE) != 0U) {
        *c = (char)(USART2->DR & 0xFFU);
        return 1U;
    }

    return 0U;
}
