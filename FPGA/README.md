# FPGA 开发说明

**开发板**：Xilinx Spartan-6 AX309
**开发语言**：Verilog HDL
**开发工具**：Xilinx ISE 14.7（免费，需注册Xilinx账号下载）

---

## FPGA在系统中的角色

FPGA只做两件事：

```
STM32 ──SPI──→ [ spi_slave.v ]  接收16位占空比数值（0~1000）
                      │
                      │ duty[9:0]
                      ▼
               [ pwm_gen.v ]    根据占空比生成20kHz PWM波形
                      │
                      │ pwm_out
                      ▼
               TB6612 PWMA      驱动电机
```

STM32是大脑，负责所有计算。FPGA是纯硬件PWM执行器。

---

## 各模块说明

### `spi_slave.v` — SPI从机接收模块

- 接收STM32（SPI主机）发来的数据
- 模式：CPOL=0，CPHA=0（时钟空闲低，上升沿采样）
- 帧格式：16位，高位先发（MSB first）
- 输出：`duty[9:0]`——接收到的16位数据的低10位，即占空比值

### `pwm_gen.v` — PWM波形生成模块

- 时钟来源：50MHz（AX309板载晶振）
- PWM频率：20kHz（周期 = 50MHz / 20kHz = 2500个时钟周期）
- 占空比分辨率：0~1000（对应0%~100%）
- duty=0 → PWM常低 → 电机停止
- duty=1000 → PWM常高 → 电机满速

### `top.v` — 顶层模块

- 实例化 `spi_slave` 和 `pwm_gen` 两个子模块
- 把 `spi_slave` 的输出 `duty_out` 连接到 `pwm_gen` 的输入 `duty`
- ISE综合时选这个文件作为顶层模块（Top Module）

---

## 引脚分配（需对照AX309原理图确认实际引脚编号）

| 信号名 | 方向 | 说明 | 连接目标 |
|--------|------|------|---------|
| `clk` | 输入 | 50MHz板载时钟 | AX309晶振 |
| `rst_n` | 输入 | 复位，低电平有效 | AX309复位按钮 |
| `spi_sclk` | 输入 | SPI时钟 | STM32 PA5 |
| `spi_mosi` | 输入 | SPI数据输入 | STM32 PA7 |
| `spi_ss` | 输入 | 片选，低电平有效 | STM32 PA4 |
| `pwm_out` | 输出 | 20kHz PWM信号 | TB6612 PWMA引脚 |

> 在ISE里创建 `.ucf` 约束文件，把上表中的信号名映射到FPGA的物理引脚编号。
> 具体哪个信号对应AX309扩展口的哪个引脚，需要查阅AX309原理图（向卖家索取或网上搜索）。

---

## ISE 14.7 编译和烧录步骤

1. 打开ISE → 新建工程 → 添加 `top.v`、`spi_slave.v`、`pwm_gen.v`
2. 设置顶层模块为 `top`
3. 创建 `.ucf` 约束文件，填写引脚约束
4. 依次运行：综合（Synthesize）→ 实现（Implement）→ 生成比特流（Generate Programming File），得到 `.bit` 文件
5. 用JTAG下载器连接AX309
6. 打开iMPACT → Boundary Scan → 烧录 `.bit` 文件到FPGA
