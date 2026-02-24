# DC Motor Speed Control System

**Author**: Yuqi Li — s336721
**Course**: Electronics for Embedded Systems — Politecnico di Torino

---

## What This System Does

The user turns a potentiometer to set a target speed.
The STM32 reads sensors, runs a cascade PID algorithm, and sends a PWM command to the FPGA.
The FPGA generates the PWM waveform to drive the motor.
The OLED shows target and actual speed in real time.

---

## Hardware Architecture

```
┌──────────────────────────────────────────────────────────────────────────┐
│  SENSING                                                                 │
│                                                                          │
│  [Potentiometer] → [RC Filter] ──────────────→ STM32 ADC (PA3, CH3)    │
│                                                                          │
│  [Motor] → [Encoder] ───────────────────────→ STM32 TIM2 encoder mode  │
│                                                                          │
│  [Motor] → [Shunt 0.1Ω] → [INA240 ×20] ───→ STM32 ADC (PA2, CH2)     │
└──────────────────────────────────────────────────────────────────────────┘
                          │ target_speed, actual_speed, actual_current
                          ▼
┌──────────────────────────────────────────────────────────────────────────┐
│  DECISION MAKING  (STM32, runs every 10 ms)                             │
│                                                                          │
│   target_speed ──→ [Outer loop: Speed PI] ──→ target_current           │
│   actual_speed ──↗                                  │                   │
│                                                      ▼                   │
│                    target_current ──→ [Inner loop: Current PI] ──→ pwm_duty (0~1000)
│                    actual_current ──↗                                    │
└──────────────────────────────────────────────────────────────────────────┘
                          │ pwm_duty via SPI
                          ▼
┌──────────────────────────────────────────────────────────────────────────┐
│  ACTUATION                                                               │
│                                                                          │
│  STM32 ──SPI──→ FPGA (Spartan-6) ──→ PWM generator ──→ TB6612 PWMA    │
│                                        20 kHz PWM                        │
│  STM32 ──GPIO──→ TB6612 AIN1/AIN2/STBY  (direction + enable)           │
│                  TB6612 ──→ Motor (12 V DC, JGA25-370)                  │
└──────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────┐
│  SUPPORT                                                                 │
│                                                                          │
│  W25Q64 Flash ──SPI──→ STM32  (PID Kp/Ki saved here, loaded at boot)  │
│  OLED SSD1306 ──I2C──→ STM32  (shows target RPM and actual RPM)        │
│  PC           ←─UART── STM32  (prints T / A / I / PWM every 10 ms)    │
└──────────────────────────────────────────────────────────────────────────┘
```

---

## Why a Cascade PID?

The professor flagged that reading just one ADC value is not non-trivial.
The solution adds **real current feedback** from the motor, enabling a two-loop controller:

```
  target_speed ──→ [Speed PI]   ──→  target_current
  actual_speed ──↗
                                       target_current ──→ [Current PI] ──→ pwm_duty
                                       actual_current ──↗
                                       (shunt → INA240 → ADC)
```

- Speed loop: slow, sets how much current the motor needs
- Current loop: fast, adjusts PWM directly based on actual current
- Benefit: load changes are corrected by the current loop before speed even drops

---

## Course Requirement Coverage

| Course Topic | How This Project Covers It |
|---|---|
| **Memories** | W25Q64 SPI Flash. Hand-written protocol: write enable → sector erase → page program → status poll → read. No library. |
| **Programmable Logic** | FPGA Spartan-6 AX309. Verilog: SPI slave (`spi_slave.v`) + 20 kHz PWM generator (`pwm_gen.v`). |
| **Interconnections** | SPI (to FPGA and Flash), I2C (OLED), UART (PC). Three protocols, all register-level. |
| **Processor Peripherals** | STM32 ADC, TIM2, SPI1, I2C1, USART1 — direct register access only. No HAL. |
| **AD/DA Conversion** | **AD**: RC hardware filter + 2-channel ADC (potentiometer + INA240 current sense) + software moving average. **DA**: FPGA PWM controls motor power analogously. |
| **Power Management** | TB6612FNG H-bridge. Controlled by FPGA PWM (speed) + STM32 GPIO (direction/enable). Drives 12 V motor. |

---

## File Structure

```
Project/
├── README.md                         ← This file
├── Electronics_Project_Proposal.pdf  ← Approved project proposal
├── 购买好的材料清单                   ← Hardware list (with missing parts noted)
│
├── STM32/                            ← MCU firmware (C, direct register access)
│   ├── README.md                     ← Full pin assignment table
│   ├── Inc/
│   │   ├── uart.h     UART debug output
│   │   ├── adc.h      ADC: potentiometer + current sense
│   │   ├── encoder.h  TIM2 encoder mode → RPM
│   │   ├── spi.h      SPI master: FPGA command + Flash protocol
│   │   ├── i2c.h      I2C + OLED SSD1306 driver
│   │   ├── motor.h    TB6612 direction and enable (GPIO)
│   │   └── pid.h      Cascade PI: speed loop + current loop
│   └── Src/
│       ├── main.c     Boot sequence + 10 ms control loop
│       ├── uart.c     ✅ Complete (Stage 1 done)
│       ├── adc.c      TODO: Stage 2 + 7
│       ├── encoder.c  TODO: Stage 3
│       ├── i2c.c      TODO: Stage 4
│       ├── spi.c      TODO: Stage 6 (FPGA) + Stage 10 (Flash)
│       ├── motor.c    TODO: Stage 6
│       └── pid.c      TODO: Stage 8 + 9
│
├── FPGA/                             ← FPGA logic (Verilog, Xilinx ISE 14.7)
│   ├── README.md                     ← Pin mapping + ISE setup guide
│   └── src/
│       ├── top.v          Top module — wires spi_slave to pwm_gen
│       ├── spi_slave.v    SPI slave — receives 16-bit duty value from STM32
│       └── pwm_gen.v      PWM generator — 20 kHz, duty 0~1000
│
└── Docs/                             ← Guides and reference
    ├── 文档资料索引.md                ← Where to download every datasheet
    ├── 硬件电路说明/
    │   └── RC低通滤波器.md            ← RC filter circuit for potentiometer ADC
    ├── 阶段1_UART调试/
    │   └── 步骤指南.md                ← Complete Stage 1 walkthrough
    └── 阶段10_Flash存储/
        └── 实现要点.md                ← W25Q64 full protocol (important for exam)
```

---

## Development Stages

Build one stage at a time. After each stage, use UART to print the value and confirm it is correct before moving on.

| Stage | Goal | Files to edit |
|-------|------|---------------|
| **1** | UART works — print "Hello" to PC serial monitor | `uart.c` ✅ |
| **2** | ADC reads potentiometer — UART prints raw value and RPM | `adc.c` |
| **3** | Encoder reads motor speed — UART prints RPM | `encoder.c` |
| **4** | OLED displays a fixed number | `i2c.c` |
| **5** | FPGA: SPI slave receives value, generates fixed-duty PWM | `spi_slave.v`, `pwm_gen.v` |
| **6** | STM32 sends PWM duty via SPI → motor spins at one speed | `spi.c`, `motor.c` |
| **7** | ADC reads INA240 current — UART prints milliamps | `adc.c` |
| **8** | Speed-only PI loop closes — motor holds target RPM | `pid.c` |
| **9** | Add current inner loop → full cascade PID working | `pid.c` |
| **10** | Flash: save PID params, reload after reboot | `spi.c` |
| **11** | Full integration: OLED live, UART live, everything stable | `main.c` |
