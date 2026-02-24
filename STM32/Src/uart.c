/*
 * uart.c — UART串口调试模块实现
 *
 * 硬件连接：
 *   STM32 PA9  (TX) → CH340 RXD
 *   STM32 PA10 (RX) → CH340 TXD
 *   STM32 GND       → CH340 GND  ← 必须共地！
 *
 * 电脑端：串口助手（SSCOM等），波特率 115200，8N1
 *
 * 时钟说明：
 *   本文件使用STM32上电后的内部时钟 HSI = 8MHz（默认值，无需配置PLL）
 *   若后续系统升级到72MHz，把 USART1->BRR 改为 0x271 即可
 *
 * 参考：RM0008 第27章（USART）
 */

#include "uart.h"
#include "stm32f10x.h"

void UART_Init(void) {
    /* ── 步骤1：打开时钟 ──────────────────────────────────────────
     * APB2ENR（APB2外设时钟使能寄存器）
     *   bit2  = IOPAEN   → 使能 GPIOA 时钟
     *   bit14 = USART1EN → 使能 USART1 时钟
     */
    RCC->APB2ENR |= (1 << 2);    /* GPIOA 时钟 */
    RCC->APB2ENR |= (1 << 14);   /* USART1 时钟 */

    /* ── 步骤2：配置引脚（CRH 控制 PA8~PA15）─────────────────────
     * 每个引脚占4位：CNF[1:0] | MODE[1:0]
     *
     * PA9（TX，输出）：复用推挽输出 50MHz
     *   CNF=10（复用推挽），MODE=11（50MHz） → 0b1011 = 0xB
     *   PA9 在 CRH 的 bit[7:4]
     *
     * PA10（RX，输入）：浮空输入
     *   CNF=01（浮空输入），MODE=00（输入）  → 0b0100 = 0x4
     *   PA10 在 CRH 的 bit[11:8]
     */
    GPIOA->CRH &= ~(0xFFU << 4);  /* 先清除 PA9 和 PA10 的原有配置 */
    GPIOA->CRH |=  (0xBU  << 4);  /* PA9：复用推挽输出（TX） */
    GPIOA->CRH |=  (0x4U  << 8);  /* PA10：浮空输入（RX） */

    /* ── 步骤3：设置波特率 ─────────────────────────────────────────
     * HSI = 8MHz，波特率 = 115200：
     *   USARTDIV = 8000000 / (16 × 115200) = 4.34
     *   整数部分 = 4       → BRR[15:4] = 4
     *   小数部分 = 0.34×16 = 5.44 → 取整5 → BRR[3:0] = 5
     *   最终 BRR = (4 << 4) | 5 = 0x45
     *
     * 若系统时钟升级到72MHz：改为 USART1->BRR = 0x271;
     */
    USART1->BRR = 0x45;

    /* ── 步骤4：使能USART ──────────────────────────────────────────
     * CR1 寄存器：
     *   bit3  = TE  → 发送使能（Transmitter Enable）
     *   bit2  = RE  → 接收使能（Receiver Enable）
     *   bit13 = UE  → USART使能（USART Enable，必须最后打开）
     */
    USART1->CR1 = (1 << 3) | (1 << 2) | (1 << 13);
}

void UART_SendChar(char c) {
    /*
     * SR寄存器 bit7 = TXE（发送数据寄存器空标志）
     * 等待 TXE=1，表示上一个字节已送入移位寄存器，可以写入新数据
     */
    while (!(USART1->SR & (1 << 7)));
    USART1->DR = (unsigned char)c;
}

void UART_SendString(const char *str) {
    while (*str) {
        UART_SendChar(*str++);
    }
}

void UART_SendInt(int val) {
    char buf[12];
    int i = 0;
    if (val < 0) {
        UART_SendChar('-');
        val = -val;
    }
    if (val == 0) {
        UART_SendChar('0');
        return;
    }
    while (val > 0) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    /* 反转输出 */
    for (int j = i - 1; j >= 0; j--) {
        UART_SendChar(buf[j]);
    }
}
