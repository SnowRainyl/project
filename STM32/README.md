# STM32 — Firmware Guide

**Chip**: STM32F103C8T6 (minimum system board, "Blue Pill")
**Rule**: Direct register access only. No HAL library, no LL library.

---

## Pin Assignment

| Function | Peripheral | STM32 Pin | Connected To |
|----------|-----------|-----------|--------------|
| Potentiometer input | ADC1_CH3 | PA3 | Potentiometer wiper (via RC filter) |
| Motor current sense | ADC1_CH2 | PA2 | INA240 output |
| Encoder channel A | TIM2_CH1 | PA0 | Motor encoder A |
| Encoder channel B | TIM2_CH2 | PA1 | Motor encoder B |
| SPI clock | SPI1_SCK | PA5 | FPGA + W25Q64 (shared) |
| SPI data out | SPI1_MOSI | PA7 | FPGA + W25Q64 (shared) |
| SPI data in | SPI1_MISO | PA6 | FPGA + W25Q64 (shared) |
| Chip select — FPGA | GPIO out | PA4 | FPGA spi_ss |
| Chip select — Flash | GPIO out | PB0 | W25Q64 CS |
| I2C clock | I2C1_SCL | PB6 | OLED SSD1306 |
| I2C data | I2C1_SDA | PB7 | OLED SSD1306 |
| UART transmit | USART1_TX | PA9 | CH340 RXD |
| UART receive | USART1_RX | PA10 | CH340 TXD |
| Motor enable | GPIO out | PB1 | TB6612 STBY |
| Motor direction A | GPIO out | PB10 | TB6612 AIN1 |
| Motor direction B | GPIO out | PB11 | TB6612 AIN2 |

> PA0/PA1 are taken by TIM2 encoder mode — potentiometer uses PA3 (ADC CH3) instead.
> FPGA outputs PWM directly to TB6612 PWMA. STM32 does not touch the PWM line.

---

## Module Summary

| File | What it does |
|------|-------------|
| `uart.c/h` | USART1: 115200 baud, register-level. Used for all debug printing. **Done — Stage 1.** |
| `adc.c/h` | ADC1: reads PA3 (potentiometer → target RPM) and PA2 (INA240 → motor current mA). Hardware RC filter + software 8-sample moving average. |
| `encoder.c/h` | TIM2 in encoder interface mode. Reads pulse count every 10 ms, converts to RPM. |
| `spi.c/h` | SPI1 master. Two functions: (1) send pwm_duty to FPGA, (2) read/write PID params to W25Q64 Flash using hand-written protocol. |
| `i2c.c/h` | I2C1 + SSD1306 OLED driver. Shows target RPM (line 1) and actual RPM (line 2). |
| `motor.c/h` | GPIO control of TB6612 AIN1/AIN2/STBY. Sets motor direction and enables the driver chip. PWM itself comes from FPGA. |
| `pid.c/h` | Cascade PI algorithm. Outer loop: speed error → target current. Inner loop: current error → pwm_duty. |
| `main.c` | Startup sequence + 10 ms control loop. |

---

## Startup Sequence (main.c)

```
UART_Init()               → UART ready, can now print debug messages
ADC_Init()                → ADC ready
Encoder_Init()            → TIM2 encoder mode ready
SPI_Init()                → SPI1 ready (FPGA and Flash both accessible)
I2C_Init() + OLED_Init()  → OLED ready
Motor_Init()              → TB6612 pins configured, motor in standby

Flash_ReadPIDParams()     → read Kp/Ki saved from last session
PID_InitAll()             → initialize speed_pid and current_pid with those values

Motor_SetForward()        → enable TB6612, set direction forward
                            (PWM starts at 0, motor stationary until PID outputs)

→ enter 10 ms control loop
```

---

## Control Loop (every 10 ms)

```
Read potentiometer  → target_speed  (RPM)
Read encoder        → actual_speed  (RPM)
Read INA240 via ADC → actual_current (mA)

CascadePID_Compute(target_speed, actual_speed, actual_current)
  → outer loop: speed_error → target_current
  → inner loop: current_error → pwm_duty (0~1000)

SPI_SendPWMDuty(pwm_duty)  → FPGA updates PWM immediately

every 100 ms: OLED_ShowSpeed(target_speed, actual_speed)
every 10 ms:  UART prints T / A / I / PWM for debugging
```
