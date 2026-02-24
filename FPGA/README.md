# FPGA — Development Guide

**Board**: Xilinx Spartan-6 AX309
**Language**: Verilog HDL
**Tool**: Xilinx ISE 14.7 (free download, requires Xilinx account)

---

## Role in the System

The FPGA does two things and only two things:

```
STM32 ──SPI──→ [ spi_slave.v ]  receives 16-bit duty value (0~1000)
                      │
                      │ duty[9:0]
                      ▼
               [ pwm_gen.v ]    generates 20 kHz PWM with that duty cycle
                      │
                      │ pwm_out
                      ▼
               TB6612 PWMA      drives the motor
```

The STM32 is the brain. The FPGA is purely a PWM hardware engine.

---

## Module Descriptions

### `spi_slave.v` — SPI Slave Receiver

- Receives data sent by the STM32 (SPI master)
- Mode: CPOL=0, CPHA=0 (clock idle low, sample on rising edge)
- Frame: 16 bits, MSB first
- Output: `duty[9:0]` — the lower 10 bits of the received 16-bit value

### `pwm_gen.v` — PWM Generator

- Clock: 50 MHz (AX309 onboard oscillator)
- PWM frequency: 20 kHz (period = 50 MHz / 20 kHz = 2500 clock cycles)
- Resolution: duty cycle 0 to 1000 (0% to 100%)
- When duty = 0 → PWM always low → motor stops
- When duty = 1000 → PWM always high → motor full speed

### `top.v` — Top Module

- Instantiates `spi_slave` and `pwm_gen`
- Connects `duty_out` from spi_slave to `duty` input of pwm_gen
- This is the synthesis entry point in ISE

---

## Pin Assignments (verify against AX309 schematic)

| Signal | Direction | Description | Connect to |
|--------|-----------|-------------|------------|
| `clk` | Input | 50 MHz board clock | AX309 oscillator |
| `rst_n` | Input | Reset, active low | AX309 button |
| `spi_sclk` | Input | SPI clock | STM32 PA5 |
| `spi_mosi` | Input | SPI data in | STM32 PA7 |
| `spi_ss` | Input | Chip select, active low | STM32 PA4 |
| `pwm_out` | Output | 20 kHz PWM signal | TB6612 PWMA |

> Map these signals to physical FPGA pins using the `.ucf` constraints file in ISE.
> Check the AX309 schematic to find which FPGA pin connects to which expansion header pin.

---

## How to Build and Program (ISE 14.7)

1. Open ISE → New Project → add `top.v`, `spi_slave.v`, `pwm_gen.v`
2. Set top module to `top`
3. Create a `.ucf` file — assign each signal to a physical FPGA pin
4. Run: Synthesize → Implement → Generate Programming File (`.bit`)
5. Connect JTAG programmer to AX309
6. Open iMPACT → Boundary Scan → Program the `.bit` file
