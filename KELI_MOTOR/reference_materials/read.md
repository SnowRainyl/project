STM32：RM0090（核心）、PM0214、UM1472、DS8626
FPGA：AX309 用户手册、DS160、UG381、Libraries Guide
电机：JGA25-370 规格书 + RM0090 TIM3 编码器模式章节
TB6612：Toshiba datasheet，重点看真值表
INA240：TI datasheet，重点看增益版本区分
SSD1306：Solomon Systech datasheet + RM0090 I2C 章节
W25Q64：Winbond datasheet，重点看指令集
CH340：沁恒 CH340DS1 + Windows 驱动下载
最核心的就一本：RM0090，STM32 所有外设寄存器都在这里，用的时候查对应章节就行。

---

# KELI_MOTOR 接线表

说明：
- “STM32 主表”保留所有 STM32 相关引脚，方便写代码时按引脚查。
- 只和 STM32 简单相连的外设，在主表和简单外设供电表里写清楚即可。
- 连接超过两个模块、涉及功率回路、采样回路、FPGA 物理排针的部件，额外单独开表。
- 所有通信和模拟采样都必须共地；`W25Q64` 当前是预留/待验证状态。

## 1. STM32 主表：按 STM32 引脚查

| STM32 引脚 | STM32 外设/功能 | 对端模块 | 对端引脚/信号 | 状态 | 用处 |
|---|---|---|---|---|---|
| `PA0` | `ADC1_IN0` | 电位器 + RC 低通 | RC 低通输出 | 已用 | 读取目标转速设定电压 |
| `PA1` | `ADC1_IN1` | INA240 | `OUT` | 已用 | 读取电机电流反馈电压 |
| `PA2` | `USART2_TX` | CH340 | `RXD` | 已用 | STM32 发送调试信息到电脑 |
| `PA3` | `USART2_RX` | CH340 | `TXD` | 已用 | STM32 接收串口数据 |
| `PA4` | `SPI1_CS` | W25Q64 | `CS` | 预留/待验证 | SPI Flash 片选 |
| `PA5` | `SPI1_SCK` | W25Q64 | `CLK` | 预留/待验证 | SPI Flash 时钟 |
| `PA6` | `SPI1_MISO` | W25Q64 | `DO/MISO` | 预留/待验证 | STM32 读取 Flash 数据 |
| `PA7` | `SPI1_MOSI` | W25Q64 | `DI/MOSI` | 预留/待验证 | STM32 写 Flash 数据 |
| `PB6` | `I2C1_SCL` | OLED SSD1306 | `SCL` | 已用 | OLED I2C 时钟 |
| `PB7` | `I2C1_SDA` | OLED SSD1306 | `SDA` | 已用 | OLED I2C 数据 |
| `PB12` | `SPI2_CS` | FPGA | `cs_n` | 已用 | STM32 到 FPGA 的 SPI 片选 |
| `PB13` | `SPI2_SCK` | FPGA | `sck` | 已用 | STM32 到 FPGA 的 SPI 时钟 |
| `PB14` | `SPI2_MISO` | FPGA | `miso` | 已接/可暂未使用 | FPGA 回传数据到 STM32 |
| `PB15` | `SPI2_MOSI` | FPGA | `mosi` | 已用 | STM32 发送 PWM 占空比给 FPGA |
| `PC6` | `TIM3_CH1` | 电机编码器 | A 相/黄线 | 已用 | 编码器 A 相脉冲计数 |
| `PC7` | `TIM3_CH2` | 电机编码器 | B 相/绿线 | 已用 | 编码器 B 相脉冲计数 |

## 2. 简单外设供电表

| 模块 | 引脚/线 | 连接到 | 注意 |
|---|---|---|---|
| OLED SSD1306 | `VCC` | `3.3V` 或按模块标注 | 常见 4 针 I2C 模块为 `GND/VCC/SCL/SDA` |
| OLED SSD1306 | `GND` | 系统 `GND` | 必须与 STM32 共地 |
| CH340 | `GND` | 系统 `GND` | 必须与 STM32 共地 |
| CH340 | `VCC` | 不接 | STM32 自有供电，避免 USB-TTL 模块反向供电 |
| W25Q64 | `VCC` | `3.3V` | 当前预留/待验证 |
| W25Q64 | `GND` | 系统 `GND` | 当前预留/待验证 |
| W25Q64 | `/WP`、`/HOLD` | 上拉到 `3.3V` | 接线时不要悬空，否则可能无法正常读写 |

## 3. FPGA 独立表：STM32 SPI2 与 PWM 输出

| FPGA 信号 | FPGA 芯片引脚 | AX309 J3 物理针脚 | 连接对象 | 代码/外设 | 用处 |
|---|---|---|---|---|---|
| `clk` | `P8` | 板载 50MHz 晶振 | - | FPGA 时钟 | PWM 与 SPI 从机逻辑时钟 |
| `cs_n` | `D16` | `J3-23` | STM32 `PB12` | `SPI2_CS` | SPI 帧片选，低有效 |
| `sck` | `C15` | `J3-21` | STM32 `PB13` | `SPI2_SCK` | SPI 时钟 |
| `mosi` | `C16` | `J3-24` | STM32 `PB15` | `SPI2_MOSI` | 接收 STM32 发来的 duty |
| `miso` | `E15` | `J3-26` | STM32 `PB14` | `SPI2_MISO` | 回传数据，当前可暂未使用 |
| `pwm_out` | `F16` | `J3-31` | TB6612 `PWMA` | FPGA PWM | 输出 12bit PWM 到电机驱动 |
| `GND` | 任意 GND 引脚 | J3 任意 GND | 系统 `GND` | - | STM32/FPGA/TB6612 必须共地 |

## 4. TB6612FNG 电机驱动表

| TB6612 引脚 | 连接到 | 信号来源/类型 | 用处 |
|---|---|---|---|
| `PWMA` | FPGA `pwm_out` / `F16` / `J3-31` | 3.3V PWM | A 路电机调速输入 |
| `AIN1` | `3.3V` | 固定高电平 | 固定电机方向 |
| `AIN2` | `GND` | 固定低电平 | 固定电机方向 |
| `STBY` | `3.3V` | 固定高电平 | 拉高使能驱动器，否则待机不工作 |
| `VCC` | `3.3V` | 逻辑电源 | TB6612 逻辑侧供电 |
| `VM` | `12V` 电机电源 | 功率电源 | 给电机主功率回路供电 |
| `GND` | 系统 `GND` | 共地 | 与 STM32、FPGA、INA240 共地 |
| `AO1/AO2` | 电机两端，串入 INA240 采样路径 | 功率输出 | 驱动直流电机转动 |

## 5. INA240A2 电流采样表

| INA240 引脚/信号 | 连接到 | 代码/参数 | 用处 |
|---|---|---|---|
| `VCC` | `3.3V` | - | INA240 芯片供电 |
| `GND` | 系统 `GND` | - | 与驱动功率地和 STM32 共地 |
| `OUT` | STM32 `PA1` | `ADC1_IN1` | 输出放大后的电流采样电压 |
| `IN+` | 板载 `R100=0.1ohm` 采样电阻一端 | `ADC_CURR_SHUNT_MOHM=100.0f` | 采样电流路径一端 |
| `IN-` | 板载 `R100=0.1ohm` 采样电阻另一端 | `ADC_CURR_SHUNT_MOHM=100.0f` | 采样电流路径另一端 |
| `REF1/REF2` | 模块内部连接 | 零电流实测约 `1773mV` | 形成双向测量零电流基准 |

当前使用的是 `INA240A2`，增益为 `50V/V`，代码应使用：

```c
#define ADC_CURR_INA240_GAIN     50.0f
#define ADC_CURR_SHUNT_MOHM      100.0f
#define ADC_CURR_VREF_MV         1773.0f
```

换算关系：

```text
V_out(mV)   = ADC_raw / 4095.0 * 3300
V_shunt(mV) = V_out - ADC_CURR_VREF_MV
I(mA)       = V_shunt / (50 * 0.1)
```

## 6. 电机与编码器线序表

| 线/信号 | 连接到 | 代码/外设 | 用处 |
|---|---|---|---|
| 编码器 A 相/黄线 | STM32 `PC6` | `TIM3_CH1` | 读取 A 相脉冲 |
| 编码器 B 相/绿线 | STM32 `PC7` | `TIM3_CH2` | 读取 B 相脉冲 |
| 编码器 V+ / 蓝线 | 按电机规格接 `3.3V` 或 `5V` | - | 编码器供电，实际电压以电机资料/实测模块为准 |
| 编码器 GND / 黑线 | 系统 `GND` | - | 编码器共地 |
| 电机动力线 1 | TB6612 `AO1` 或经 INA240 采样路径 | - | 电机主功率回路 |
| 电机动力线 2 | TB6612 `AO2` 或经 INA240 采样路径 | - | 电机主功率回路 |

注意：编码器黄/绿/蓝/黑线是反馈与供电线，电机红/白线是动力线，两类线不要混接。

## 7. 总链路核对表

| 链路 | 从 | 到 | 用处 |
|---|---|---|---|
| 目标转速输入 | 电位器中间抽头 | RC 低通 -> STM32 `PA0` | ADC 读取目标转速 |
| 速度反馈 | 编码器 A/B 相 | STM32 `PC6/PC7` | TIM3 编码器模式测 RPM |
| 电流反馈 | 电机支路电流 -> R100 -> INA240A2 | STM32 `PA1` | ADC 读取电流反馈 |
| PWM 命令 | STM32 `PB12/PB13/PB15` | FPGA `cs_n/sck/mosi` | SPI2 发送 duty |
| FPGA 回传 | FPGA `miso` | STM32 `PB14` | 当前可预留 |
| PWM 输出 | FPGA `pwm_out` | TB6612 `PWMA` | FPGA 生成 PWM 驱动电机 |
| 功率驱动 | TB6612 `AO1/AO2` | 电机两端 | 电机主功率回路 |
| 显示 | STM32 `PB6/PB7` | OLED `SCL/SDA` | 显示 RPM、Duty、电流 |
| 串口调试 | STM32 `PA2/PA3` | CH340 `RXD/TXD` | 电脑查看调试信息 |
| 数据存储 | STM32 `PA4~PA7` | W25Q64 | 预留保存 PID 参数、日志 |

## 初学者最容易混的几条

### 1. `PA0` 读的是什么
- `PA0 -> ADC1_IN0`
- 读的是“电位器经 RC 低通滤波后的模拟电压”
- 不是直接读电阻值

链路：

```text
电位器中间抽头 -> RC低通 -> PA0 -> ADC -> 数字值(0~4095)
```

### 2. `PA1` 读的是什么
- `PA1 -> ADC1_IN1`
- 读的是 `INA240 OUT` 输出的模拟电压
- 这个电压对应电机支路电流大小

链路：

```text
电机电流 -> R100(0.1ohm) -> INA240A2放大 -> OUT -> PA1 -> ADC
```

### 3. 编码器线和电机动力线不是一回事
- 黄/绿：编码器 A/B 相信号，给 STM32 `PC6/PC7`
- 蓝/黑：编码器供电
- 红/白：电机动力线，给 `TB6612 AO1/AO2`

### 4. INA240 不是给电机供电
- `INA240 VCC` 只是给放大器芯片自己供电
- 真正让电机转起来的是外部 `12V -> TB6612 -> 电机`

### 5. 你这个电流采样更接近什么类型
- 不是最标准的“高边”或“低边”教材接法
- 按当前接线，更接近 `H桥输出侧 / in-line` 电流采样

## 最常查的代码入口

- 编码器：`MOTOR_TEST/MDK-ARM/Src/encoder_reg.h/.c`
- ADC（电位器、电流）：`MOTOR_TEST/MDK-ARM/Src/adc_reg.h/.c`
- SPI（STM32 到 FPGA / Flash）：`MOTOR_TEST/MDK-ARM/Src/spi_Reg.h/.c`
- 控制主循环：`MOTOR_TEST/Core/Src/stm32f4xx_it.c`


