# DC Motor Speed Control System
**作者**: Yuqi Li - s336721
**课程**: Electronics Project - Politecnico di Torino

---

## 项目简介

用电位器设定目标转速，STM32读取传感器数据，运行串级PID算法，通过SPI把PWM指令发给FPGA，FPGA生成PWM波驱动电机，OLED实时显示转速。

---

## 系统架构

```
[电位器] → [RC滤波] → [STM32 ADC]
                              ↓
[编码器] ──────────→ [STM32 Timer] → [串级PID算法] → [SPI] → [FPGA] → [PWM] → [电机驱动TB6612] → [电机]
                              ↑                                                        ↓
[分流电阻] → [INA240放大] → [STM32 ADC]                                            [编码器反馈]
                              ↓
                         [OLED显示] (I2C)
                         [PC调试]  (UART)
                         [Flash存储] (SPI)
```

**数据流说明：**
1. 电位器 → ADC 读取目标转速
2. 编码器 → Timer 计数 → 计算实际转速
3. 分流电阻 → INA240 → ADC 读取实际电流
4. 串级PID：外环（速度）→ 内环（电流）→ 输出PWM占空比
5. STM32 via SPI → FPGA，FPGA生成精确PWM
6. PWM → TB6612 → 驱动电机
7. STM32 via I2C → OLED显示，via UART → PC调试

---

## 文件结构

```
Project/
├── README.md               ← 你现在在看的这个文件
├── 购买好的材料清单         ← 硬件材料清单
│
├── STM32/                  ← MCU固件代码（C语言，不用HAL库）
│   ├── README.md           ← STM32开发说明
│   ├── Inc/                ← 头文件（.h）
│   │   ├── adc.h           ← ADC模块接口
│   │   ├── encoder.h       ← 编码器测速接口
│   │   ├── spi.h           ← SPI通信接口
│   │   ├── i2c.h           ← I2C驱动OLED接口
│   │   ├── uart.h          ← UART串口调试接口
│   │   └── pid.h           ← 串级PID算法接口
│   └── Src/                ← 源文件（.c）
│       ├── main.c          ← 主程序入口
│       ├── adc.c           ← ADC初始化和采样
│       ├── encoder.c       ← 编码器读取和测速
│       ├── spi.c           ← SPI主机通信
│       ├── i2c.c           ← I2C + OLED显示
│       ├── uart.c          ← UART调试输出
│       └── pid.c           ← 串级PID算法实现
│
└── FPGA/                   ← FPGA逻辑代码（Verilog）
    ├── README.md           ← FPGA开发说明
    └── src/
        ├── top.v           ← 顶层模块（连接各子模块）
        ├── spi_slave.v     ← SPI从机接收模块
        └── pwm_gen.v       ← PWM波形生成模块
```

---

## 开发顺序建议（适合初学者）

| 阶段 | 任务 | 涉及文件 |
|------|------|---------|
| 1 | UART调试先跑通，之后可以用它打印调试信息 | `uart.c` |
| 2 | ADC读取电位器，UART打印结果验证 | `adc.c` |
| 3 | 编码器计数，UART打印转速验证 | `encoder.c` |
| 4 | OLED显示数字（先显示固定值） | `i2c.c` |
| 5 | FPGA：SPI从机 + PWM生成（先固定占空比验证） | `spi_slave.v`, `pwm_gen.v` |
| 6 | STM32 SPI发送，电机转起来 | `spi.c` |
| 7 | 电流采样：分流电阻 + INA240 → ADC | `adc.c` |
| 8 | 实现单环PID（先只做速度环） | `pid.c` |
| 9 | 加入电流内环，完成串级PID | `pid.c` |
| 10 | Flash读写PID参数 | `spi.c` |
| 11 | 整合所有模块，联调 | `main.c` |
