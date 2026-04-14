# STM32F407 × FPGA 电机控制开发记录

> 项目路径：`KELI_MOTOR/MOTOR_TEST`
> MCU：STM32F407（寄存器级编程）
> 日期：2026-03-07

---

## 一、SPI2 测试方案（F407 ↔ G474 Nucleo）

### 1.1 现有 SPI2 配置参数（来自 `MDK-ARM/Src/spi_Reg.c`）

| 参数 | 值 |
|---|---|
| 引脚 | PB12=CS, PB13=SCK, PB14=MISO, PB15=MOSI |
| 时钟源 | APB1 = 42 MHz |
| 分频 | /8 → **5.25 MHz** |
| 模式 | Mode 0（CPOL=0, CPHA=0） |
| 数据帧 | 8-bit，MSB First |
| NSS | 软件管理（GPIO 手动拉低） |
| 角色 | **Master** |

### 1.2 Step 1：自回环测试（最快验证，无需第二块板）

**硬件**：杜邦线短接 **PB14 (MISO) ↔ PB15 (MOSI)**

```c
// USER CODE BEGIN 2
uint8_t test_tx[] = {0xAA, 0x55, 0xA5, 0x5A};
uint8_t test_rx[4] = {0};
uint8_t loopback_ok = 1;

FPGA_CS_LOW();
for (int i = 0; i < 4; i++) {
    test_rx[i] = SPI2_ReadWriteByte(test_tx[i]);
}
FPGA_CS_HIGH();

for (int i = 0; i < 4; i++) {
    if (test_rx[i] != test_tx[i]) { loopback_ok = 0; break; }
}
// 预期输出：[SPI2 Loopback] TX:AA55A55A RX:AA55A55A PASS
```

### 1.3 Step 2：G474 Nucleo 双板测试

#### 接线表

| STM32F407 (SPI2 Master) | NUCLEO-G474RE (SPI1 Slave) |
|---|---|
| PB12  CS/NSS | PA4   SPI1_NSS   (CN10-32) |
| PB13  SCK    | PA5   SPI1_SCK   (CN10-11) |
| PB15  MOSI   | PA7   SPI1_MOSI  (CN10-15) |
| PB14  MISO   | PA6   SPI1_MISO  (CN10-13) |
| GND          | GND |

> MOSI → MOSI，MISO → MISO（命名从 Master 视角，方向自然相反）

#### G474 CubeMX 配置

```
SPI1:
  Mode:            Full-Duplex Slave
  Hardware NSS:    Hardware NSS Input Signal
  Data Size:       8 Bits
  First Bit:       MSB First
  Clock Polarity:  Low   (CPOL=0)
  Clock Phase:     1 Edge (CPHA=0)

USART2:
  Mode:      Asynchronous
  Baud Rate: 115200
```

#### G474 测试代码（CubeMX HAL 生成工程）

```c
uint8_t spi_rx[4] = {0};
uint8_t spi_tx[4] = {0x11, 0x22, 0x33, 0x44};
char print_buf[64];

while (1) {
    HAL_StatusTypeDef ret = HAL_SPI_TransmitReceive(&hspi1, spi_tx, spi_rx, 4, 5000);
    if (ret == HAL_OK) {
        snprintf(print_buf, sizeof(print_buf),
            "[G474 Slave] RX: %02X %02X %02X %02X\r\n",
            spi_rx[0], spi_rx[1], spi_rx[2], spi_rx[3]);
        HAL_UART_Transmit(&huart2, (uint8_t*)print_buf, strlen(print_buf), 100);
    }
}
```

#### 预期结果

| 串口 | 预期输出 |
|---|---|
| F407 UART | `[F407 SPI2] TX:AA 55 A5 5A \| RX(from G474):11 22 33 44` |
| G474 UART2 | `[G474 Slave] RX: AA 55 A5 5A` |

---

## 二、ADC1 寄存器配置（读取电位器）

### 2.1 硬件资源

```
PA0 → ADC1_IN0（模拟输入）

电位器接法：
  3.3V ──┬── 电位器一端
         ├── 中心抽头 → PA0
  GND  ──┴── 电位器另一端
```

### 2.2 时钟配置

```
APB2 = 84MHz
ADC_CCR ADCPRE = /4 → ADC 时钟 = 21MHz（≤ 36MHz 限制 ✓）
采样时间 = 84 周期 → 约 4μs（适合电位器高阻抗源）
分辨率 = 12位，输出范围 0~4095
```

### 2.3 `adc_reg.h`

```c
#ifndef __ADC_REG_H
#define __ADC_REG_H

#include "stm32f4xx.h"

void     ADC1_Init(void);
uint16_t ADC1_Read(void);   /* 返回 12 位原始值 0~4095 */

#endif
```

### 2.4 `adc_reg.c`

```c
#include "adc_reg.h"

void ADC1_Init(void) {
    // 1. 开启时钟
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;

    // 2. PA0 模拟输入（MODER=11）
    GPIOA->MODER |= (3U << (0 * 2));

    // 3. ADC 时钟分频：APB2/4 = 21MHz
    ADC->CCR &= ~ADC_CCR_ADCPRE;
    ADC->CCR |=  (1U << 16);          // 01: /4

    // 4. CR1：12位分辨率（默认）
    ADC1->CR1 = 0;

    // 5. CR2：单次转换，软件触发，右对齐
    ADC1->CR2 = 0;
    ADC1->CR2 |= ADC_CR2_ADON;

    // 6. 序列：仅转换 1 个通道（IN0）
    ADC1->SQR1 = 0;   // L=0 → 1次
    ADC1->SQR3 = 0;   // SQ1=CH0

    // 7. 采样时间：84 周期（SMP0=110）
    ADC1->SMPR2 &= ~(7U << (0 * 3));
    ADC1->SMPR2 |=  (6U << (0 * 3));
}

uint16_t ADC1_Read(void) {
    ADC1->CR2 |= ADC_CR2_SWSTART;
    while (!(ADC1->SR & ADC_SR_EOC));
    return (uint16_t)(ADC1->DR & 0x0FFF);
}
```

---

## 三、SPI2 数据协议（MCU → FPGA）

### 3.1 帧格式（2字节，12bit 占空比）

```
CS↓
  Byte 0: [7:4]=0000  [3:0]=adc_val[11:8]   ← 高4位
  Byte 1: [7:0]=adc_val[7:0]                ← 低8位
CS↑

FPGA 端还原：duty_reg[11:0] = {byte0[3:0], byte1[7:0]}
```

**设计理由**：
- CS 边沿天然作为帧定界符，无需同步头
- 保留完整 12 位精度
- FPGA 状态机极简（3个状态）

### 3.2 FPGA SPI 从机状态机骨架（Verilog）

```verilog
always @(posedge clk) begin
    case (state)
        S_IDLE:    if (!cs_n) state <= S_RECV_HI;
        S_RECV_HI: if (spi_rx_valid) begin
                       temp_hi <= spi_rx_byte[3:0];
                       state   <= S_RECV_LO;
                   end
        S_RECV_LO: if (spi_rx_valid) begin
                       duty_reg <= {temp_hi, spi_rx_byte}; // 12bit
                       state    <= S_IDLE;
                   end
    endcase
end

// PWM 比较器（计数器 0~4095）
assign pwm_out = (pwm_cnt < duty_reg);
```

---

## 四、系统架构：PID 跑在 MCU

```
电位器
  │
  ▼
ADC1 (PA0)  ─→  setpoint (0~4095)
                     │
                     ▼
              [PID_Calc() @ 1kHz]  ←── feedback（暂时=0，等编码器接入）
                     │
                     ▼
              duty (0~4095, 12bit)
                     │
              SPI2（PB12~PB15）
                     │
                     ▼
              FPGA → PWM → 电机
```

**控制分工**：

| 模块 | 负责 |
|---|---|
| STM32F407 | ADC采样、PID计算、SPI2发送 |
| FPGA | 接收占空比、生成PWM、（后续）编码器计数回传 |

---

## 五、PID 控制器

### 5.1 `pid.h`

```c
#ifndef __PID_H
#define __PID_H

typedef struct {
    float kp, ki, kd;
    float integral;
    float err_prev;
    float out_min, out_max;
    float integral_max;
} PID_TypeDef;

void  PID_Init(PID_TypeDef *pid,
               float kp, float ki, float kd,
               float out_min, float out_max,
               float integral_max);

float PID_Calc(PID_TypeDef *pid, float setpoint, float feedback);
void  PID_Reset(PID_TypeDef *pid);

#endif
```

### 5.2 `pid.c`（位置式，带积分限幅+输出限幅）

```c
#include "pid.h"

void PID_Init(PID_TypeDef *pid,
              float kp, float ki, float kd,
              float out_min, float out_max, float integral_max)
{
    pid->kp = kp; pid->ki = ki; pid->kd = kd;
    pid->out_min = out_min; pid->out_max = out_max;
    pid->integral_max = integral_max;
    pid->integral = 0.0f; pid->err_prev = 0.0f;
}

float PID_Calc(PID_TypeDef *pid, float setpoint, float feedback)
{
    float err = setpoint - feedback;

    pid->integral += err;
    if      (pid->integral >  pid->integral_max) pid->integral =  pid->integral_max;
    else if (pid->integral < -pid->integral_max) pid->integral = -pid->integral_max;

    float output = pid->kp * err
                 + pid->ki * pid->integral
                 + pid->kd * (err - pid->err_prev);
    pid->err_prev = err;

    if      (output > pid->out_max) output = pid->out_max;
    else if (output < pid->out_min) output = pid->out_min;
    return output;
}

void PID_Reset(PID_TypeDef *pid) {
    pid->integral = 0.0f;
    pid->err_prev = 0.0f;
}
```

---

## 六、TIM6 1kHz 控制环（`stm32f4xx_it.c`）

### 6.1 TIM6 时钟计算

```
APB1_CLK = 42MHz（APB1 预分频=4）
TIM6_CLK = 42 × 2 = 84MHz（APB1预分频≠1时，TIM时钟翻倍）
PSC = 83  → 84MHz / 84 = 1MHz
ARR = 999 → 1MHz / 1000 = 1kHz（1ms 周期）
```

### 6.2 `Motor_Control_Init()`

```c
void Motor_Control_Init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;

    TIM6->PSC  = 83;
    TIM6->ARR  = 999;
    TIM6->CNT  = 0;
    TIM6->DIER |= TIM_DIER_UIE;

    NVIC_SetPriority(TIM6_DAC_IRQn, 2);
    NVIC_EnableIRQ(TIM6_DAC_IRQn);

    TIM6->CR1 |= TIM_CR1_CEN;

    // 暂无编码器：kp=1, ki=0, kd=0 → 直通模式（output=setpoint）
    PID_Init(&motor_pid, 1.0f, 0.0f, 0.0f, 0.0f, 4095.0f, 2048.0f);
}
```

### 6.3 `TIM6_DAC_IRQHandler()`（1kHz 控制环主体）

```c
void TIM6_DAC_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF) {
        TIM6->SR &= ~TIM_SR_UIF;   // 清中断标志

        uint16_t setpoint = ADC1_Read();

        // feedback=0.0f 占位，编码器接入后替换
        float    duty_f   = PID_Calc(&motor_pid, (float)setpoint, 0.0f);
        uint16_t duty     = (uint16_t)duty_f;

        uint8_t hi = (duty >> 8) & 0x0F;
        uint8_t lo =  duty       & 0xFF;

        FPGA_CS_LOW();
        SPI2_ReadWriteByte(hi);
        SPI2_ReadWriteByte(lo);
        FPGA_CS_HIGH();

        g_pid_duty = duty;
    }
}
```

**ISR 执行时间估算**：

| 操作 | 耗时 |
|---|---|
| ADC 转换（84周期@21MHz） | ≈ 4.6 μs |
| SPI 2字节（@5.25MHz） | ≈ 3.0 μs |
| 其余开销 | < 1.0 μs |
| **合计** | **≈ 9 μs / 1000 μs = 0.9% CPU** |

---

## 七、向量表机制说明

```
编译时：startup_stm32f407xx.s 向量表
        DCD TIM6_DAC_IRQHandler   ← 链接器用你的强符号覆盖弱符号

运行时：Motor_Control_Init() "开门"
        ├─ TIM6->CR1 |= CEN        开始计数
        ├─ TIM6->DIER |= UIE       允许TIM6发中断请求
        └─ NVIC_EnableIRQ()        允许CPU响应

每 1ms：TIM6溢出 → 硬件查向量表 → 跳到 TIM6_DAC_IRQHandler
         ISR执行完毕 → 硬件恢复现场 → 继续执行 main()
```

main() 的 `HAL_Delay(200)` 完全不影响控制环，两者并发执行。

---

## 八、接入编码器后的修改（仅需改一行）

在 `TIM6_DAC_IRQHandler` 中：

```c
// 改前（无反馈）：
float duty_f = PID_Calc(&motor_pid, (float)setpoint, 0.0f);

// 改后（接入编码器，encoder_speed 为实测转速）：
float duty_f = PID_Calc(&motor_pid, (float)setpoint, (float)encoder_speed);
```

然后重新整定 kp、ki、kd（通过修改 `Motor_Control_Init` 里的 `PID_Init` 参数）。

---

## 九、工程文件清单

| 文件 | 位置 | 说明 |
|---|---|---|
| `spi_Reg.h/.c` | `MDK-ARM/Src/` | SPI1(Flash) + SPI2(FPGA) 寄存器驱动 |
| `adc_reg.h/.c` | `MDK-ARM/Src/` | ADC1 寄存器驱动（PA0，电位器） |
| `pid.h/.c`     | `MDK-ARM/Src/` | 位置式PID控制器 |
| `stm32f4xx_it.c` | `Core/Src/` | TIM6 1kHz中断 + PID控制环 |
| `main.c`       | `Core/Src/` | 初始化调用 + 监控打印 |

> **Keil 注意**：`adc_reg.c` 和 `pid.c` 需手动添加到工程文件组。
