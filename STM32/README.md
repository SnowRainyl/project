# STM32 固件开发说明

芯片：STM32F103C8T6（最小系统板）
开发要求：**直接操作寄存器**，不使用 HAL 库

---

## 引脚分配规划

| 功能           | 外设      | STM32 引脚        | 连接目标                  |
|----------------|-----------|-------------------|---------------------------|
| 电位器采样     | ADC1_CH1  | PA1               | 电位器中间脚（经RC滤波）  |
| 电流采样       | ADC1_CH2  | PA2               | INA240 输出               |
| 编码器A相      | TIM2_CH1  | PA0               | 电机编码器A               |
| 编码器B相      | TIM2_CH2  | PA1               | 电机编码器B               |
| SPI to FPGA    | SPI1      | PA5(CLK) PA7(MOSI) PA4(NSS) | FPGA SPI从机  |
| SPI to Flash   | SPI1      | 共用CLK/MOSI，PB0(NSS) | W25Q64              |
| OLED I2C       | I2C1      | PB6(SCL) PB7(SDA) | OLED SSD1306              |
| UART 调试      | USART1    | PA9(TX) PA10(RX)  | USB-TTL CH340             |

> 注意：PA0和PA1被TIM2编码器占用后，电位器需换到其他ADC通道（如PA3）。
> 最终引脚分配需根据实际焊接情况确认。

---

## 模块说明

- **adc.c/h** — 配置ADC1，读取电位器电压（通道1）和电流信号（通道2）
- **encoder.c/h** — 配置TIM2为编码器模式，定时读取计数值换算转速(RPM)
- **spi.c/h** — 配置SPI1为主机模式，发送PWM占空比给FPGA，读写W25Q64 Flash
- **i2c.c/h** — 配置I2C1，驱动OLED SSD1306显示目标速度和实际速度
- **uart.c/h** — 配置USART1，printf重定向，打印PID调试信息
- **pid.c/h** — 串级PID：外环（速度环）+ 内环（电流环）
- **main.c** — 初始化所有外设，主循环协调各模块
