# STM32 固件开发说明

芯片：STM32F103C8T6（最小系统板）
开发要求：**直接操作寄存器**，不使用 HAL 库

---

## 引脚分配规划

| 功能              | 外设      | STM32 引脚               | 连接目标                    |
|-------------------|-----------|--------------------------|------------------------------|
| 电位器采样        | ADC1_CH3  | PA3                      | 电位器中间脚（经RC滤波）    |
| 电流采样          | ADC1_CH2  | PA2                      | INA240 输出                 |
| 编码器A相         | TIM2_CH1  | PA0                      | 电机编码器A                 |
| 编码器B相         | TIM2_CH2  | PA1                      | 电机编码器B                 |
| SPI 时钟          | SPI1_SCK  | PA5                      | FPGA + W25Q64（共享）       |
| SPI 主发从收      | SPI1_MOSI | PA7                      | FPGA + W25Q64（共享）       |
| SPI 主收从发      | SPI1_MISO | PA6                      | FPGA + W25Q64（共享）       |
| SPI 片选 FPGA     | GPIO      | PA4                      | FPGA SS引脚                 |
| SPI 片选 Flash    | GPIO      | PB0                      | W25Q64 CS引脚               |
| OLED I2C 时钟     | I2C1_SCL  | PB6                      | OLED SSD1306                |
| OLED I2C 数据     | I2C1_SDA  | PB7                      | OLED SSD1306                |
| UART 发送         | USART1_TX | PA9                      | CH340 RXD                   |
| UART 接收         | USART1_RX | PA10                     | CH340 TXD                   |
| 电机使能（STBY）  | GPIO      | PB1                      | TB6612 STBY                 |
| 电机方向 AIN1     | GPIO      | PB10                     | TB6612 AIN1                 |
| 电机方向 AIN2     | GPIO      | PB11                     | TB6612 AIN2                 |

> **PA0/PA1 已被 TIM2 编码器占用**，电位器改接 PA3（ADC1 通道3）。
> FPGA 输出 PWM → TB6612 PWMA（不经过STM32）。

---

## 模块说明

- **uart.c/h** — USART1寄存器配置，串口调试输出（阶段1，已完成）
- **adc.c/h** — ADC1配置，读取电位器（PA3）和电流（PA2），含软件滑动平均滤波
- **encoder.c/h** — TIM2编码器模式，读取脉冲计数换算RPM
- **spi.c/h** — SPI1主机，发PWM占空比给FPGA；手写W25Q64 Flash读写协议
- **i2c.c/h** — I2C1配置，驱动OLED SSD1306显示目标/实际转速
- **motor.c/h** — TB6612 AIN1/AIN2/STBY GPIO控制（方向和使能，PWM由FPGA控制）
- **pid.c/h** — 串级PI：外环（速度环）→ 内环（电流环）→ PWM占空比
- **main.c** — 启动序列 + 主控制循环（10ms周期）
