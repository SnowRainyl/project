/*
 * main.c — 主程序
 *
 * DC Motor Speed Control System
 * STM32F103C8T6，直接寄存器操作，不用HAL库
 *
 * 启动流程：
 *   初始化外设 → 从Flash读取PID参数 → 初始化PID → 电机待机 → 进入主循环
 *
 * Flash存储用途：
 *   启动时读取上次保存的PID参数（Kp/Ki），避免每次上电都用默认值。
 *   若Flash为空（首次上电），Flash_ReadPIDParams()自动返回默认参数。
 *   调好PID后，调用 Flash_WritePIDParams() 保存，下次上电自动生效。
 */

#include "uart.h"
#include "adc.h"
#include "encoder.h"
#include "spi.h"
#include "i2c.h"
#include "pid.h"
#include "motor.h"   /* TB6612方向控制 */

static void delay_ms(int ms) {
    for (int i = 0; i < ms * 8000; i++) {
        __asm("nop");
    }
}

int main(void) {
    /* ===== 1. 初始化所有外设 ===== */
    UART_Init();
    UART_SendString("System starting...\r\n");

    ADC_Init();
    Encoder_Init();
    SPI_Init();      /* SPI初始化后，Flash（W25Q64）和FPGA均可访问 */
    I2C_Init();
    OLED_Init();
    OLED_Clear();
    Motor_Init();    /* 配置TB6612的AIN1/AIN2/STBY引脚，初始待机 */

    /* ===== 2. 从Flash读取PID参数，初始化PID控制器 =====
     *
     * 数据流：W25Q64 Flash
     *              ↓ Flash_ReadPIDParams()
     *         kp_speed, ki_speed, kp_current, ki_current
     *              ↓ PID_InitAll()
     *         speed_pid 控制器 + current_pid 控制器（在pid.c里）
     *
     * 第一次上电Flash为空时，Flash_ReadPIDParams()返回spi.c里的默认值。
     */
    float kp_speed, ki_speed, kp_current, ki_current;
    Flash_ReadPIDParams(&kp_speed, &ki_speed, &kp_current, &ki_current);
    PID_InitAll(kp_speed, ki_speed, kp_current, ki_current);
    UART_SendString("PID loaded from Flash.\r\n");

    /* ===== 3. 设置电机方向并使能驱动 =====
     * 本项目默认正转（电位器控制速度，不控制方向）
     * PWM=0时电机静止，PWM随PID输出从0逐渐增大
     */
    Motor_SetForward();
    UART_SendString("Motor forward. Entering control loop.\r\n");

    /* ===== 4. 主控制循环 ===== */
    int loop_counter = 0;

    while (1) {
        /* --- 感知（Sensing） --- */
        int target_speed   = ADC_ReadTargetSpeed();   /* 单位：RPM */
        int actual_speed   = Encoder_GetSpeedRPM();   /* 单位：RPM */
        int actual_current = ADC_ReadMotorCurrent();  /* 单位：mA  */

        Encoder_Update();  /* 更新编码器计数，计算本周期速度 */

        /* --- 决策（Decision-Making） --- */
        int pwm_duty = CascadePID_Compute(target_speed, actual_speed, actual_current);

        /* --- 执行（Actuation） --- */
        SPI_SendPWMDuty((unsigned short)pwm_duty);   /* 发给FPGA → 生成PWM → TB6612 → 电机 */

        /* --- 显示，每100ms更新一次OLED --- */
        if (loop_counter % 10 == 0) {
            OLED_ShowSpeed(target_speed, actual_speed);
        }

        /* --- UART调试输出 --- */
        UART_SendString("T:");  UART_SendInt(target_speed);
        UART_SendString(" A:"); UART_SendInt(actual_speed);
        UART_SendString(" I:"); UART_SendInt(actual_current);
        UART_SendString(" PWM:"); UART_SendInt(pwm_duty);
        UART_SendString("\r\n");

        /*
         * --- 保存PID参数到Flash（TODO：后期实现触发机制）---
         * 调好PID后，可通过以下调用永久保存：
         *   Flash_WritePIDParams(kp_speed, ki_speed, kp_current, ki_current);
         */

        loop_counter++;
        delay_ms(10);  /* 控制周期 10ms */
    }
}
