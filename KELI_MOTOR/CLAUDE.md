# KELI_MOTOR — Claude 上下文接力手册

> 新窗口打开时请先读完本文件，再看代码或提问。本文件是项目的"活记录"，每次有重大进展时更新。

---

## 用户背景与沟通方式（必读）

- 用户是**计算机专业硕士**，有软件/CS 基础
- **已知**：寄存器读写、状态机基本概念（能记录历史状态，有 VHDL 状态机经验）
- **需要解释**：
  - 电子/硬件/电路相关术语（增益、共模电压、上拉电阻、电容、差分信号等）
  - 通信协议的物理/时序细节（SPI/I2C/UART 帧结构、时钟极性等）
  - 状态机的深入用法（超出"记录历史状态"基础概念的部分）
  - 嵌入式专有缩写（CMRR、BRR、ARR、PSC 等）
- 解释风格：**一句话核心 + 必要时类比**，不堆砌术语，鼓励追问

---

## 项目一句话概述

STM32F407（寄存器级 C）+ AX309 FPGA（VHDL）的直流电机**串级闭环控制**系统。
速度外环 + 电流内环，目标 0~100 RPM 平滑可控，由旋钮电位器给定目标转速。

---

## 仓库结构

```
KELI_MOTOR/
├── MOTOR_TEST/                  ← 主工程（Keil MDK-ARM）
│   ├── Core/Src/                ← HAL 核心：main.c, stm32f4xx_it.c（TIM6 中断）
│   ├── MDK-ARM/Src/             ← 寄存器级自写驱动（见下表）
│   │   ├── spi_Reg.c/h          ← SPI1（Flash） + SPI2（FPGA）
│   │   ├── adc_reg.c/h          ← ADC1：PA0 电位器, PA1 电流
│   │   ├── encoder_reg.c/h      ← TIM3 编码器模式（PC6/PC7）
│   │   ├── pid.c/h              ← 位置式 PID，积分限幅+输出限幅
│   │   ├── uart.c/h             ← USART2（PA2=TX, PA3=RX）
│   │   ├── custom_i2c.c/h       ← I2C1（PB6=SCL, PB7=SDA）
│   │   ├── OLED_SSD1306.c/h     ← OLED 驱动
│   │   ├── w25q64.c/h           ← W25Q64FV SPI Flash 驱动
│   │   └── Font_CN/EN.c, FontLib.h
│   ├── spi_slave.vhd            ← FPGA SPI Slave（接收12bit占空比）
│   ├── pwm_gen.vhd              ← FPGA PWM 发生器（50MHz/4096 ≈ 12.2kHz）
│   ├── top.vhd                  ← FPGA 顶层
│   ├── 开发记录_SPI2_ADC_PID.md ← 详细技术笔记（SPI/ADC/PID实现细节）
│   ├── 硬件接线与调试记录.md    ← 所有接线表+调试历史+PID参数
│   └── 硬件部件清单.md          ← 12个硬件组件的规格/用途/资料
├── pwm_vivado/                  ← Vivado 版 PWM 工程（AX309 不用此目录）
└── 资料手册/                    ← 各模块官方资料
```

---

## Git Commit 历史（从旧到新）

| Commit | 内容 |
|--------|------|
| `d005a4a` | 项目骨架初始化：STM32 驱动骨架 + FPGA Verilog 骨架 |
| `221bc83` | 补全 motor.h/motor.c（TB6612 方向控制），修复课程要求缺口 |
| `1d6556e` | 重写三份 README，加分层架构图和串级 PID 说明 |
| `b7ef92d` | README 全部改为中文 |
| `0781ae1` | 加入 MOTOR_TEST Keil 工程（真正的代码开始在这里） |
| `08e3fc8` | 修复 W25Q64 驱动：ReadID 协议、操作顺序、CS 时序 |
| `3124eb3` | 加入 pwm_vivado（Vivado 版，AX309 不用） |
| `e9c70ff` | **主要里程碑**：完整寄存器级驱动 + VHDL + 串级 PID 全部就位 |
| `2051a27` | 记录 ST-Link VCP 无输出问题及调试结论 |
| `472a788` | 重组已知问题章节，分离已修复/未解决 |
| `21c6e98` | **切换串口**：弃用 ST-Link VCP，改用 CH340G（PA2/PA3，COM12） |
| `93f9752` | 新增硬件部件清单（12个组件） |
| `c759d1f` | 补全清单资料来源，修正 Flash 状态为"未接线待验证" |

---

## 当前功能状态（截至 2026-05-12）

### 已验证 ✅

| 功能 | 说明 |
|------|------|
| FPGA PWM 输出 | 逻辑分析仪实测 12.207kHz，与 50MHz/4096 完全吻合 |
| OLED 显示 | 实时显示 RPM/Duty/Current，I2C 超时保护已修复 |
| 编码器转速读取 | TIM3 PC6/PC7，0~100 RPM 线性响应正常 |
| 电流采样 | INA240A2（增益50V/V），空载 ~30mA，与实测吻合 |
| 串级 PID 闭环 | 速度 0~100 RPM 平滑可控，无振荡 |
| 串口调试 | CH340G 接 PA2/PA3，COM12，115200 稳定收发 |
| SPI1 自回环 | MOSI 短接 MISO 测试 PASS |
| **Flash W25Q64 读写持久化** | ID=0xEF16，PID 参数 save→断电→上电自动加载，全链路验证通过 |

### 待解决 ❌

| 问题 | 现象 | 排查方向 |
|------|------|----------|
| **SPI2 自回环 FAIL（已定位根因）** | RX 全为 0x00 | `spi_slave.vhd` 第47行 `miso <= '0'` 硬编码拉低，FPGA 上电后持续驱动 MISO=0V，与自回环短接线信号争夺并获胜。**实际电机控制不受影响**（SPI2 是单向写，FPGA 不需要回传数据）。验证方法：拔掉 J3-26（FPGA MISO 线），再做短接自回环应 PASS；或直接用示波器测 J3-31 PWM 占空比随电位器变化来确认端到端正常。 |
| 电流环带负载测试 | 仅空载测试过 | 加负载后观察内环响应 |

---

## 关键硬件参数（快速查阅）

### 引脚分配

| 外设 | STM32 引脚 | 说明 |
|------|-----------|------|
| ADC1_IN0  | PA0  | 电位器（目标转速，RC 滤波后接入） |
| ADC1_IN1  | PA1  | INA240 输出（电流反馈） |
| USART2_TX | PA2  | → CH340G RXD |
| USART2_RX | PA3  | ← CH340G TXD |
| SPI1_CS   | PA4  | W25Q64 Flash 片选（GPIO 输出，软件 NSS） |
| SPI1_SCK  | PA5  | W25Q64 Flash 时钟（AF5） |
| SPI1_MISO | PA6  | W25Q64 Flash 数据输出（AF5） |
| SPI1_MOSI | PA7  | W25Q64 Flash 数据输入（AF5） |
| I2C1_SCL  | PB6  | OLED SCL（AF4，开漏+上拉） |
| I2C1_SDA  | PB7  | OLED SDA（AF4，开漏+上拉） |
| SPI2_CS   | PB12 | FPGA cs_n（GPIO 输出，软件 NSS） |
| SPI2_SCK  | PB13 | FPGA sck（AF5） |
| SPI2_MISO | PB14 | FPGA miso（AF5，FPGA 硬编码驱动 0，可不接） |
| SPI2_MOSI | PB15 | FPGA mosi（AF5） |
| TIM3_CH1  | PC6  | 编码器 A 相（AF2，上拉输入） |
| TIM3_CH2  | PC7  | 编码器 B 相（AF2，上拉输入） |
| GND × 5   | GND（多点） | CH340G / FPGA J3 / 编码器 / INA240 / OLED 各一根，必须共地 |

### FPGA 引脚分配（AX309 J3 扩展口）

> 工具：ISE 14.7，约束文件 `pin.ucf`

| 信号 | FPGA 引脚 | J3 物理针脚 | 对端 |
|------|-----------|------------|------|
| clk（50MHz） | P8 | 板载晶振 | — |
| sck  | C15 | J3-21 | STM32 PB13（SPI2_SCK） |
| cs_n | D16 | J3-23 | STM32 PB12（SPI2_CS） |
| mosi | C16 | J3-24 | STM32 PB15（SPI2_MOSI） |
| miso | E15 | J3-26 | STM32 PB14（SPI2_MISO，FPGA 硬编码输出 0，可不接） |
| pwm_out | F16 | J3-31 | → TB6612 PWMA |

### 串级 PID 当前参数

| 环 | kp | ki | kd | out_max |
|----|----|----|-----|---------|
| 速度外环 | 1.5 | 0.1 | 0.0 | 400 mA |
| 电流内环 | 5.0 | 0.2 | 0.0 | 4095（12bit满量程） |

- 控制频率：TIM6 1kHz 中断（PSC=83, ARR=999, TIM6_CLK=84MHz）
- MOTOR_MAX_RPM = 100 RPM（电位器满量程对应值）
- ADC 死区：adc < 50 → setpoint = 0（消除零点漂移）

### INA240A2 电流换算

```
V_shunt = (ADC_raw / 4095) × 3300mV - 1773mV   ← 1773mV 为实测零电流校准值
I (mA)  = V_shunt / (50 × 0.1Ω)                ← 增益50V/V, 采样电阻0.1Ω
```

---

## 已踩过的坑（新窗口必读，避免重复踩）

| 坑 | 教训 |
|----|------|
| ST-Link VCP 串口无输出 | F407G-DISC1 板 ST-Link VCP 链路异常，**永远用 CH340G**（PA2/PA3） |
| INA240 增益版本 | 模块是 **A2（50V/V）**，不是 A1（20V/V）；搞错会导致电流读数偏高 2.5 倍 |
| 编码器不供电 | 编码器无 5V 供电时 RPM=0，速度环积分飙升，duty 拉满 → **先查编码器供电** |
| OLED 卡死 | I2C 事务不要关闭 PE；需加超时保护 + 清 AF/BERR/ARLO/OVR |
| ADC 死区 | 电位器归零时 adc≈9，不加死区会导致积分累积偶发抖转 |
| AX309 FPGA 工具链 | Spartan-6 **只能用 ISE 14.7**，不支持 Vivado；烧录用 iMPACT JTAG，断电丢失 |
| Flash ID=0xFFFF | **MISO 未接线**时必然全 FF，不是驱动问题，先接线 |
| Flash 写入静默失败 | 接线后 save 显示成功但掉电丢失，回读 raw=FFFFFFFF，擦除后 WEL=1 stuck → 根因是 **tSHSL 时序违反**（CS↑ 到下一条 CS↓ 间隔 < 50ns，芯片静默拒绝写/擦命令）；修复：在 Write_Enable / Erase_Sector / Write_4Floats 的 CS↑ 后加 `HAL_Delay(1)` |
| SPI2 自回环不能带 FPGA 测 | `spi_slave.vhd` 的 `miso <= '0'` 让 FPGA 持续驱动 MISO=0V；做自回环必须先拔掉 J3-26 那根线断开 FPGA MISO；正常使用时 SPI2 是单向写，MISO 线可以不接 |

---

## 下一步工作（优先级排序）

1. **带负载测试电流环**，观察内环对扰动的抑制效果，必要时重新整定参数
2. **重新烧录 FPGA** → 再测 SPI2 STM32→FPGA 通信（自回环用 PB14 短接 PB15，需先拔掉 J3-26）

> ~~接 W25Q64 Flash 物理线并验证 ReadID~~ ← **已完成**（2026-05-12）：ID=0xEF16，tSHSL 时序修复后读写/持久化全部验证通过

---

## 开发工具

- **IDE**：Keil MDK-ARM（工程文件：`MOTOR_TEST/MDK-ARM/MOTOR_TEST.uvprojx`）
- **FPGA 综合**：ISE 14.7（Spartan-6 专用）
- **烧录调试**：ST-Link（仅烧录/SWD调试）+ FPGA JTAG iMPACT
- **串口**：CH340G → COM12（设备管理器中找 "USB-SERIAL CH340"），115200 8N1
- **资料手册**：`资料手册/` 目录分类存放各模块 datasheet 和参考手册

---

## 实验报告结构

> 参考同学报告风格，结合本项目特点定制。报告核心亮点是**串级 PID 控制理论 + STM32/FPGA 协同实现**，第 4 章必须有数学推导。

```
Abstract
Acknowledgments
Contents
List of Tables
List of Figures

1  System Analysis and Design
   1.1  Functional Requirements and Feasibility Analysis
   1.2  System Architecture Design
        — 分层架构图（Sensing / Decision / Actuation / Support）
   1.3  Control Strategy: Why Cascade PID
        — 单环 vs 串级对比，选串级的理由

2  Hardware Development
   2.1  Microcontroller System (STM32F407G-DISC1)
        2.1.1  MCU selection and specifications
        2.1.2  Pin allocation and peripheral mapping
   2.2  FPGA PWM Generation Module (AX309 Spartan-6)
        2.2.1  Role of FPGA in the system
        2.2.2  PWM frequency derivation  ← 数学推导：50MHz/4096 ≈ 12.2kHz
   2.3  Motor Drive Circuit (TB6612FNG)
        — H 桥原理，AIN1/AIN2/STBY 控制逻辑
   2.4  Sensing System
        2.4.1  Speed sensing: encoder + TIM3 quadrature decoding
               ← 转速公式推导：delta×60000/PPR
        2.4.2  Current sensing: INA240A2 signal chain
               ← 换算公式：V_shunt / (增益×采样电阻)，零偏校准 1773mV
        2.4.3  Speed setpoint: potentiometer + RC low-pass filter

3  Embedded Software Development
   3.1  Register-level Peripheral Drivers
        3.1.1  UART — CH340G serial debug (USART2, PA2/PA3, 115200)
        3.1.2  ADC — dual-channel acquisition (PA0 potentiometer, PA1 current)
        3.1.3  SPI1 — W25Q64 Flash storage driver
        3.1.4  SPI2 — FPGA communication protocol (2-byte 12-bit duty frame)
        3.1.5  I2C1 — OLED SSD1306 display (PB6/PB7)
        3.1.6  TIM3 — quadrature encoder interface (PC6/PC7, encoder mode 3)
   3.2  Signal Conditioning
        3.2.1  Moving average filter: 16-point (ADC), 8-point (encoder RPM, current)
        3.2.2  Dead zone: adc < 50 → setpoint = 0（防零点漂移）
        3.2.3  TIM3 hardware digital filter (IC1F/IC2F, 2-sample)

4  Cascade PID Control Algorithm        ← 报告核心章节，需要数学深度
   4.1  PID Mathematical Model
        — 位置式离散化公式推导
        — 积分限幅（anti-windup）原理
   4.2  Cascade Structure Design
        — 内外环带宽关系（内环 ≥ 3~5× 外环）
        — 速度外环：setpoint=RPM → output=电流给定(mA)
        — 电流内环：setpoint=mA → output=PWM占空比(0~4095)
   4.3  Parameter Tuning
        — kp/ki/kd 选取依据和整定过程（含踩坑：kp=20 振荡 → 降到 5）
   4.4  1kHz Control Loop Implementation
        — TIM6 ISR 执行时序（ADC→Encoder→SpeedPID→CurrentPID→SPI2）
        — ISR 执行时间估算（≈9μs，占周期 0.9%）

5  FPGA Development
   5.1  SPI Slave Receiver (spi_slave.vhd)
        — 2级同步链消除跨时钟域亚稳态
        — 接收状态机：SCK上升沿移位，CS上升沿锁存
        — 帧格式：2字节拼接12bit duty
   5.2  PWM Generator (pwm_gen.vhd)
        — 12bit计数器比较器原理
        — duty_valid 锁存防止毛刺
   5.3  Top-level Integration (top.vhd)

6  System Integration and Testing
   6.1  Debugging Process and Lessons Learned
        — 历史问题表（ST-Link VCP、INA240增益版本、电流环振荡、OLED卡死等）
   6.2  Experimental Results
        6.2.1  PWM frequency verification（逻辑分析仪实测 12.207kHz）
        6.2.2  Speed control performance（0~100 RPM 响应，稳态误差 < 1%）
        6.2.3  Current sensing accuracy（空载 ~30mA，与铭牌一致）
   6.3  Conclusion and Future Work
        — Flash 读写待验证，带载电流环测试，SPI2 MISO 可不接结论
```

### 各章对应的现有素材

| 章节 | 可直接复用的文件 |
|------|----------------|
| Ch1 系统设计 | CLAUDE.md 架构图、硬件部件清单.md |
| Ch2 硬件 | 硬件接线与调试记录.md §1~§5 |
| Ch3 软件驱动 | 开发记录_SPI2_ADC_PID.md §2~§6，各 .c 文件头注释 |
| Ch4 PID | stm32f4xx_it.c Motor_Control_Init + TIM6_DAC_IRQHandler，pid.c |
| Ch5 FPGA | spi_slave.vhd + pwm_gen.vhd 代码直接贴图 |
| Ch6 调试测试 | 硬件接线与调试记录.md §8~§10（已完成/未解决/历史问题） |

### 报告撰写进度（report/main.tex）

> LaTeX 模板位于 `report/main.tex`，Overleaf 上传后选 pdfLaTeX 编译。

**已写好，直接用（约 80%）：**

| 部分 | 说明 |
|------|------|
| Abstract | 完整，含真实数据（12.207kHz、30mA、<1%误差） |
| Ch1 系统分析与设计 | 功能需求、架构设计、串级PID选型理由 |
| Ch2.1 MCU | 外设表、引脚分配表（16个信号引脚） |
| Ch2.2 FPGA | PWM频率推导公式（50MHz/4096≈12.207kHz） |
| Ch2.3 TB6612 | 接线表、工作原理 |
| Ch2.4 传感器 | 编码器转速公式、INA240换算公式推导（含零偏校准1773mV） |
| Ch3 软件驱动 | UART BRR计算、ADC配置、SPI2帧协议、I2C、TIM3编码器 |
| Ch3 信号滤波 | 16/8点均值滤波表、死区说明 |
| Ch4 串级PID | 位置式离散PID推导、积分限幅、速度环/电流环参数表、ISR执行序列 |
| Ch5 FPGA | 跨时钟域同步链（VHDL代码）、接收状态机、PWM比较器、引脚表 |
| Ch6 调试历史 | 6条问题完整记录（ST-Link/INA240增益/PID振荡/OLED/编码器/死区） |
| 附录 | TIM6\_DAC\_IRQHandler + PID\_Calc 完整代码 |
| 参考文献 | 7条（RM0090、INA240、W25Q64、TB6612、SSD1306、PID教材） |

**必须用户自己补（约 20%）：**

| 待补内容 | 原因 | 建议做法 |
|----------|------|---------|
| 封面信息 | 大学名/学号/导师 | 直接填写 |
| Acknowledgments | 个人致谢 | 自己写 |
| **Fig 1.1 系统架构图** | 需要画图 | draw.io 画分层框图（Sensing/Decision/Actuation），导出PNG |
| **Fig 4.1 串级PID框图** | 需要画图 | draw.io 画标准控制框图（ref→误差→PID→执行器→反馈） |
| **Fig 2.2 INA240信号链** | 需要画图 | 简单方框图：电机→采样电阻→INA240→ADC |
| **Fig 6.1 速度响应曲线** | 需要实测数据 | 串口打印RPM数据→Python/Excel画图（最重要的实验图） |
| **Table 6.2 稳态误差表** | 需要实测 | 在20/50/80/100RPM各测3次，填入误差 |
| Ch3.3 W25Q64 那节 | 芯片未接线 | 接线验证后补写 |

**图片放置方式**：在 `report/` 目录下新建 `figures/` 文件夹，图片命名后用
`\includegraphics[width=0.85\textwidth]{figures/xxx.png}` 替换对应的 `\fbox` 占位符。

---

## 新窗口接手清单

打开新 Claude 窗口时，请按顺序做：

1. 读本文件（已完成）
2. 读 `MOTOR_TEST/硬件接线与调试记录.md`（接线 + 当前调试状态）
3. 读 `MOTOR_TEST/开发记录_SPI2_ADC_PID.md`（驱动实现细节）
4. 用 `git log --oneline` 确认最新 commit
5. 根据上方"下一步工作"列表，与用户确认本次窗口的目标

> 本文件由 Claude 维护，每次有重大进展（功能验证、bug 修复、硬件更改）后请更新对应章节。
