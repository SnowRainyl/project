# STM32 固件开发说明

**芯片**：STM32F103C8T6（最小系统板，俗称"蓝板"）
**规则**：直接操作寄存器，不使用HAL库，不使用LL库

---

## 引脚分配

| 功能 | 外设 | STM32引脚 | 连接目标 |
|------|------|-----------|---------|
| 电位器采样 | ADC1_CH3 | PA3 | 电位器中间脚（经RC滤波器） |
| 电机电流采样 | ADC1_CH2 | PA2 | INA240放大器输出 |
| 编码器A相 | TIM2_CH1 | PA0 | 电机编码器A |
| 编码器B相 | TIM2_CH2 | PA1 | 电机编码器B |
| SPI时钟 | SPI1_SCK | PA5 | FPGA + W25Q64（共用） |
| SPI主发从收 | SPI1_MOSI | PA7 | FPGA + W25Q64（共用） |
| SPI主收从发 | SPI1_MISO | PA6 | FPGA + W25Q64（共用） |
| SPI片选—FPGA | GPIO输出 | PA4 | FPGA spi_ss引脚 |
| SPI片选—Flash | GPIO输出 | PB0 | W25Q64 CS引脚 |
| I2C时钟 | I2C1_SCL | PB6 | OLED SSD1306 |
| I2C数据 | I2C1_SDA | PB7 | OLED SSD1306 |
| UART发送 | USART1_TX | PA9 | CH340 RXD |
| UART接收 | USART1_RX | PA10 | CH340 TXD |
| 电机使能 | GPIO输出 | PB1 | TB6612 STBY |
| 电机方向A | GPIO输出 | PB10 | TB6612 AIN1 |
| 电机方向B | GPIO输出 | PB11 | TB6612 AIN2 |

> PA0/PA1已被TIM2编码器占用，电位器改接PA3（ADC通道3）。
> FPGA直接输出PWM到TB6612 PWMA，不经过STM32。

---

## 各模块功能

| 文件 | 功能说明 |
|------|---------|
| `uart.c/h` | USART1：115200波特率，寄存器级配置。所有调试信息都通过它打印。**阶段1已完成。** |
| `adc.c/h` | ADC1：读PA3（电位器→目标RPM）和PA2（INA240→电机电流mA）。含RC硬件滤波+8次采样软件平均。 |
| `encoder.c/h` | TIM2编码器接口模式。每10ms读一次计数器差值，换算成RPM。 |
| `spi.c/h` | SPI1主机。两个功能：①向FPGA发送PWM占空比；②手写W25Q64协议读写PID参数。 |
| `i2c.c/h` | I2C1 + SSD1306 OLED驱动。第1行显示目标转速，第2行显示实际转速。 |
| `motor.c/h` | GPIO控制TB6612的AIN1/AIN2/STBY。设置方向和使能。PWM本身来自FPGA。 |
| `pid.c/h` | 串级PI算法。外环：速度误差→目标电流。内环：电流误差→PWM占空比。 |
| `main.c` | 启动序列 + 10ms控制循环。 |

---

## 启动流程（main.c）

```
UART_Init()               → 串口就绪，之后所有步骤都可以打印调试信息
ADC_Init()                → ADC就绪
Encoder_Init()            → TIM2编码器模式就绪
SPI_Init()                → SPI就绪（FPGA和Flash均可访问）
I2C_Init() + OLED_Init()  → OLED就绪
Motor_Init()              → TB6612引脚配置完成，电机待机

Flash_ReadPIDParams()     → 从Flash读取上次保存的Kp/Ki参数
PID_InitAll()             → 用读出的参数初始化 speed_pid 和 current_pid

Motor_SetForward()        → 使能TB6612，设置正转方向
                            （此时PWM=0，等PID计算后才开始转）

→ 进入10ms控制循环
```

---

## 控制循环（每10ms执行一次）

```
读电位器  → target_speed（单位：RPM）
读编码器  → actual_speed（单位：RPM）
读INA240 → actual_current（单位：mA）

CascadePID_Compute(target_speed, actual_speed, actual_current)
  → 外环：速度误差 → target_current（目标电流）
  → 内环：电流误差 → pwm_duty（0~1000）

SPI_SendPWMDuty(pwm_duty)  → 发给FPGA，FPGA立即更新PWM波形

每100ms：OLED_ShowSpeed(target_speed, actual_speed)
每 10ms：UART打印 T / A / I / PWM，供调试观察
```
