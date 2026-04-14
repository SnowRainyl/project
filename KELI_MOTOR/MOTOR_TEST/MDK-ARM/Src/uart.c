/*
 * uart.c — UART串口调试模块实现
 *
 * 硬件：STM32F407G-DISC1（板载ST-Link/V2-A，支持虚拟串口）
 *
 * 硬件连接：
 *   只需一根 Mini-USB 线接到板子左上角的 ST-Link USB 口（CN1）
 *   无需CH340，无需杜邦线
 *
 *   ST-Link内部已将以下引脚连接到虚拟COM口：
 *   STM32 PA2 (USART2_TX) → ST-Link → 电脑虚拟COM口
 *   STM32 PA3 (USART2_RX) → ST-Link → 电脑虚拟COM口
 *
 * 电脑端：串口助手（SSCOM等），波特率 115200，8N1
 *   COM口号：设备管理器中 "STMicroelectronics Virtual COM Port (COMx)"
 *
 * 时钟说明：
 *   F407上电默认使用内部时钟 HSI = 16MHz（不同于F103的8MHz）
 *   USART2 挂在 APB1 总线，默认 APB1 = 16MHz
 *   若后续系统升级到168MHz（APB1=42MHz），把 USART2->BRR 改为 0x16D 即可
 *
 * 参考：RM0090 第30章（USART）
 */

#include "uart.h"
#include "stm32f4xx.h"

void UART_Init(void) {
    /* ── 步骤1：打开时钟 ──────────────────────────────────────────
     * F407与F103区别：GPIO时钟在AHB1总线（不是APB2）
     *
     * AHB1ENR（AHB1外设时钟使能寄存器）
     *   bit0 = GPIOAEN → 使能 GPIOA 时钟
     *
     * APB1ENR（APB1外设时钟使能寄存器）
     *   bit17 = USART2EN → 使能 USART2 时钟（F407的USART2在APB1，不是APB2）
     */
    RCC->AHB1ENR |= (1 << 0);    /* GPIOA 时钟 */
    RCC->APB1ENR |= (1 << 17);   /* USART2 时钟 */

    /* ── 步骤2：配置引脚为复用功能（AF7 = USART2）──────────────
     * F407与F103区别：GPIO配置用 MODER + AFR，不是CRH/CRL
     *
     * 引脚：PA2（TX = USART2_TX），PA3（RX = USART2_RX）
     * 这两个引脚在板子内部连接到ST-Link芯片，形成虚拟串口
     *
     * MODER寄存器：每个引脚占2位
     *   00=输入  01=输出  10=复用功能  11=模拟
     *   PA2在bit[5:4]，PA3在bit[7:6] → 都设为 10
     *
     * AFR[0]（AFRL，控制PA0~PA7的复用功能选择）：每个引脚占4位
     *   AF7 = 0111 = USART2（见F407数据手册Table 9）
     *   PA2在bit[11:8]，PA3在bit[15:12]
     */
    GPIOA->MODER &= ~((3 << 4) | (3 << 6));   /* 清除PA2、PA3模式位 */
    GPIOA->MODER |=  ((2 << 4) | (2 << 6));   /* PA2、PA3 = 复用功能(10) */

    GPIOA->AFR[0] &= ~((0xFU << 8) | (0xFU << 12));  /* 清除PA2、PA3复用选择 */
    GPIOA->AFR[0] |=  ((7U   << 8) | (7U   << 12));  /* PA2、PA3 = AF7(USART2) */

    /* ── 步骤3：设置波特率 ─────────────────────────────────────────
     * F407默认 HSI = 16MHz（注意：F103是8MHz）
     * USART2 挂在 APB1，默认 APB1 = 16MHz
     *
     * 波特率计算：
     *   USARTDIV = fAPB1 / (16 × BaudRate)
     *            = 16,000,000 / (16 × 115,200)
     *            = 8.6805...
     *
     * BRR寄存器拆分（过采样16倍，OVER8=0）：
     *   DIV_Mantissa（整数部分）= 8      → BRR[15:4] = 8
     *   DIV_Fraction（小数部分）= 0.6805×16 = 10.89 → 取整11 → BRR[3:0] = 11
     *   最终 BRR = (8 << 4) | 11 = 0x8B
     *
     * 验证：实际波特率 = 16MHz / (16 × 8.6875) = 115,108 bps（误差0.08%，合格）
     *
     * 若APB1升级到42MHz（168MHz系统时钟）：改为 USART2->BRR = 0x16D;
     */
    USART2->BRR = 0x16D;

    /* ── 步骤4：使能USART ──────────────────────────────────────────
     * CR1 寄存器（与F103相同）：
     *   bit3  = TE  → 发送使能（Transmitter Enable）
     *   bit2  = RE  → 接收使能（Receiver Enable）
     *   bit13 = UE  → USART使能（USART Enable，必须最后打开）
     */
    USART2->CR1 = (1 << 3) | (1 << 2) | (1 << 13);
}

void UART_SendChar(char c) {
    /*
     * SR寄存器 bit7 = TXE（发送数据寄存器空标志）
     * 等待 TXE=1，表示上一个字节已送入移位寄存器，可以写入新数据
     * （F407与F103的SR/DR寄存器结构相同）
     */
    while (!(USART2->SR & (1 << 7)));
    USART2->DR = (unsigned char)c;
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
