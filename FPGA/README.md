# FPGA 开发说明

开发板：Xilinx Spartan-6 AX309
开发语言：Verilog HDL
开发工具：Xilinx ISE 14.7

---

## 功能模块

FPGA在本系统中作为 **SPI从机 + PWM生成器**：

```
           SPI信号（来自STM32）
           SCLK / MOSI / SS
                  ↓
          [ spi_slave.v ]    ← SPI从机接收，解析PWM占空比值
                  ↓
           duty[9:0]（0~1000）
                  ↓
          [ pwm_gen.v ]      ← 根据占空比生成PWM波形
                  ↓
           pwm_out           → 输出到TB6612电机驱动
```

---

## 引脚说明（需对照AX309原理图确认实际引脚）

| 信号         | 方向   | 说明                          |
|--------------|--------|-------------------------------|
| `spi_sclk`   | 输入   | SPI时钟（来自STM32 PA5）      |
| `spi_mosi`   | 输入   | SPI数据（来自STM32 PA7）      |
| `spi_ss`     | 输入   | SPI片选（来自STM32 PA4，低有效）|
| `pwm_out`    | 输出   | PWM信号（到TB6612 PWMA引脚）  |
| `clk`        | 输入   | FPGA主时钟（AX309板载50MHz）  |
| `rst_n`      | 输入   | 复位（低有效）                |

---

## 文件说明

- **top.v** — 顶层模块，连接spi_slave和pwm_gen
- **spi_slave.v** — SPI从机接收：接收STM32发来的16位数据，解析为10位占空比值
- **pwm_gen.v** — PWM生成：计数器方式，频率和分辨率可配置
