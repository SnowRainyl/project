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

---

## 十、W25Q64 Flash 驱动与 PID 参数持久化

### 10.1 硬件资源

| 引脚 | 功能 | 说明 |
|---|---|---|
| PA4 | SPI1_CS（软件 NSS） | GPIO 输出，手动控制片选 |
| PA5 | SPI1_SCK（AF5） | SPI1 时钟 |
| PA6 | SPI1_MISO（AF5） | Flash → MCU 数据 |
| PA7 | SPI1_MOSI（AF5） | MCU → Flash 数据 |

芯片：**W25Q64FV**，8MB NOR Flash，JEDEC ID = `0xEF16`（厂商 0xEF，设备 0x16）。

### 10.2 驱动接口（`w25q64.h / w25q64.c`）

| 函数 | 说明 |
|---|---|
| `W25Q64_ReadID()` | 发 0x90 指令读厂商+设备ID，正常返回 `0xEF16` |
| `W25Q64_Unprotect()` | 写 SR1=0x00, SR2=0x00，解除全片写保护 |
| `W25Q64_Erase_Sector(addr)` | 4KB 扇区擦除（指令 0x20），tSE max 400ms |
| `W25Q64_Write_4Floats(addr, pf)` | 页编程写入 4 个 float（16字节），指令 0x02 |
| `W25Q64_Read_4Floats(addr, pf)` | 标准读（指令 0x03）读取 4 个 float |
| `W25Q64_ReadSR1()` | 读状态寄存器1：bit0=BUSY，bit1=WEL |
| `W25Q64_ReadSR2()` | 读状态寄存器2：bit6=CMP，bit1=QE |

内部辅助函数（`static`）：

- `W25Q64_Wait_Busy()`：轮询 SR1.BUSY 直到为 0，设有 200000 次超时
- `W25Q64_Write_Enable()`：发 WriteEnable（0x06），使 WEL=1

### 10.3 PID 参数存储格式

Flash 扇区 0（地址 `0x000000`），存储 8 个 float（32 字节，跨两个 16 字节页编程事务）：

```
offset  0: magic     = 12345.678f   ← 标识符，全擦后值为 0xFFFFFFFF，不等于 magic
offset  4: speed_kp
offset  8: speed_ki
offset 12: speed_kd
offset 16: current_kp
offset 20: current_ki
offset 24: current_kd
offset 28: 0.0f（保留）
```

上电时 `PID_LoadFromFlash()` 自动执行，读 magic，匹配则加载参数，否则打印"使用默认值"。无需手动发 `load` 命令。

串口命令：

| 命令 | 功能 |
|---|---|
| `save` | 将当前 kp/ki/kd 写入 Flash，附带回读验证 |
| `load` | 从 Flash 手动重新加载（上电已自动执行） |
| `show` | 打印当前内存中的 PID 参数 |
| `set sp/si/sd <val>` | 修改速度外环 kp/ki/kd |
| `set cp/ci/cd <val>` | 修改电流内环 kp/ki/kd |

### 10.4 排查过程：写入静默失败

#### 现象

发 `save` 后串口输出"已保存到Flash"，但断电重启后 `load` 显示"Flash无有效参数，使用默认值"。

#### 第一步：加回读验证

在 `PID_SaveToFlash` 写完后立即 `W25Q64_Read_4Floats` 回读，打印实际读回的 float 值和原始 hex：

```
[PID] Flash写入失败！readback=nan raw=FFFFFFFF
```

`0xFFFFFFFF` 是 NOR Flash 擦除态默认值，说明**写入从未真正发生**，不是掉电丢失的问题。

#### 第二步：WEL 诊断

写入前/后分别读 SR1，观察 WEL 位（bit1）：

- 正常流程：`Write_Enable` → WEL=1 → 发 Erase 指令 → 擦除开始 → BUSY 结束 → **WEL 自动清 0**
- 异常现象：擦除命令发出后，`SR1 = 0x02`（**WEL=1 保持**）

**WEL=1 stuck 的含义**：WEL 只在成功接受并完成一次写/擦操作后才清 0。WEL 未清说明芯片**拒绝接受了擦除命令**，操作被静默忽略。

同样现象出现在启动测试的扇区 1 擦除上，说明问题不限于某个地址，而是系统性失败。

#### 第三步：排查 SR2=0x3E

读出 SR2 = `0x3E`：

```
bit7: SUS=0
bit6: CMP=0
bit5: LB3=1  ← OTP 安全寄存器锁位（一次性写入）
bit4: LB2=1
bit3: LB1=1
bit2: 保留=1
bit1: QE=1   ← Quad Enable（禁用 /HOLD 引脚）
bit0: SRP1=0
```

LB1~LB3=1 是 OTP 安全寄存器永久锁定位，**不影响主 Flash 阵列的读写**，只保护安全寄存器区域（256字节）。CMP=0、BP=000，无块保护。**SR2=0x3E 不是写失败的原因**。

#### 第四步：定位根因——tSHSL 时序违反

查阅 W25Q64FV Datasheet §7.6 AC 电气特性，找到关键参数：

> **tSHSL**（/CS Deselect Time，for Erase or Program → Read Status Registers）= **50 ns 最小值**

即：发完 WREN 指令（CS↑）到紧接着发 Erase 命令（CS↓）之间，必须至少等待 50ns。同理，发完 Erase 命令（CS↑）到紧接着读 SR1（CS↓）也需要 50ns。

原代码中没有任何延迟：

```c
// 原 Write_Enable（违反 tSHSL）
FLASH_CS_LOW();
SPI1_ReadWriteByte(W25X_WriteEnable);
FLASH_CS_HIGH();
// ← 0 延迟，立刻返回到调用处发下一条命令（CS↓ ~30ns 后）
```

STM32F407 @ 168MHz，一条指令 ~6ns，几条语句下来 CS 间隔约 30ns，不足 50ns。W25Q64 检测到违反时序，**静默丢弃了后续的写/擦命令**，但 WEL 仍维持，看起来像"写入成功了但其实没有"。

#### 修复方案

在 CS↑ 和下一条命令之间加 `HAL_Delay(1)`（1ms，远大于 50ns 最小值）：

**`W25Q64_Write_Enable`**（修复 WREN→下一命令 间隔）：

```c
static void W25Q64_Write_Enable(void) {
    HAL_Delay(1);   /* tSHSL: 上一事务结束 → 本次 CS LOW 间隔 >= 100ns */
    FLASH_CS_LOW();
    SPI1_ReadWriteByte(W25X_WriteEnable);
    FLASH_CS_HIGH();
    HAL_Delay(1);   /* tSHSL: WREN 结束 → 下一命令 CS LOW 间隔 >= 100ns */
}
```

**`W25Q64_Erase_Sector`**（修复 Erase→ReadSR1 间隔）：

```c
FLASH_CS_HIGH();
HAL_Delay(1);              /* tSHSL: Erase→ReadSR 间隔 ≥ 50ns */
W25Q64_Wait_Busy();        /* 等待擦除完成，max 400ms */
```

**`W25Q64_Write_4Floats`**（修复 PageProgram→ReadSR1 间隔）：

```c
FLASH_CS_HIGH();
HAL_Delay(1);              /* tSHSL: PageProgram→ReadSR 间隔 ≥ 50ns */
W25Q64_Wait_Busy();        /* 等待页编程完成，max 3ms */
```

> `HAL_Delay` 可用的原因：`main()` 中 `HAL_Init()` 已初始化 SysTick，项目其余驱动用寄存器级编写，但共用这一个毫秒计时基础设施，无冲突。

### 10.5 验证结果

修复后串口输出：

```
[Flash] Erase done  SR1=0x00 WEL=0(OK)
[PID] SR1=0x00 SR2=0x3E (WEL=0 BP=0 CMP=0 SRP1=0)
[PID] 已保存到Flash（验证OK，sp_kp=1.5000）
```

断电重启后自动输出：

```
[PID] Flash magic=12345.6777 raw=8FC6464A
[PID] 已从Flash加载参数
```

参数持久化完整验证通过。
