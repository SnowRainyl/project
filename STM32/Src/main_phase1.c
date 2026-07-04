/*
 * main_phase1.c — 阶段一：UART 调试通道验证
 *
 * 目标：STM32 上电后，通过 PA9(TX) 向电脑连续发送调试信息
 *       电脑用串口助手（115200, 8N1）看到输出 → 阶段一完成
 *
 * 硬件连接：
 *   STM32 PA9  (TX) → CH340 RXD
 *   STM32 PA10 (RX) → CH340 TXD
 *   STM32 GND       → CH340 GND  ← 必须共地！
 *
 * 编译时只需要：
 *   uart.c + main_phase1.c + startup + stm32f10x.h
 *   其他模块 (adc.c, spi.c ...) 全部不需要
 */

#include "uart.h"

/* 简单延时（8MHz HSI，约1ms） */
static void delay_ms(int ms) {
    for (int i = 0; i < ms * 800; i++) {
        __asm("nop");
    }
}

int main(void) {
    UART_Init();

    /* ── 上电标志 ───────────────────────────────────────────────────── */
    UART_SendString("\r\n");
    UART_SendString("=================================\r\n");
    UART_SendString("  DC Motor Control — Phase 1\r\n");
    UART_SendString("  UART OK @ 115200 baud\r\n");
    UART_SendString("=================================\r\n");

    /* ── 主循环：每500ms发一条心跳，证明MCU在正常运行 ────────────────── */
    int count = 0;
    while (1) {
        UART_SendString("Heartbeat #");
        UART_SendInt(count);
        UART_SendString("\r\n");
        count++;
        delay_ms(500);
    }
}
