# ADA.pdf 内容转写（Markdown）

> 说明：以下按页面顺序转写手写笔记。原件中个别手写符号、单词或图中细节难以完全辨认，已用 `[?]` 标注；公式按可辨识内容尽量保留原结构。

---

## Page 1 - Noise

### Aliasing

$$
SNR_e=\frac{\text{signal}}{\text{noise}}=\frac{M}{N}
=\frac{M}{M\left(\frac{f_s-f_B}{f_c}\right)^{-n_{poles}}}
=\left(\frac{f_s-f_B}{f_c}\right)^{n_{poles}}
$$

$$
SNR_e\big|_{dB}=20\,n_{poles}\,\log_{10}(\cdot)
$$

### Quantization

$$
A_e=\frac{S}{N}
\qquad
\left|E_{quant}\right|=\frac{\Delta}{2}
$$

- **Triangular signal**, range $S$:

  $$
  SNR_q\big|_{dB}>6n
  $$

- **Sinusoidal signal**, range $S$:

  $$
  SNR_q\big|_{dB}=6n+1.76
  $$

- **Square signal**, range $S$:

  $$
  SNR_q\big|_{dB}=6n+4.77
  $$

- **Gaussian distribution**:

  $$
  SNR_q\big|_{dB}=6n-4.77
  $$

### Calculating total SNR

$$
SNR_{tot}=10\log_{10}\frac{P_s}{P_{Ntot}}
=-10\log_{10}\left(\sum_i 10^{-SNR_i/10}\right)
$$

### ENOB

For full-scale sinusoidal input:

$$
ENOB=\frac{SNR-1.76}{6}
$$

> **[本项目 — ADC 精度]**: STM32F407 ADC1 是 12-bit，理论 SNR（正弦输入）= 6×12 + 1.76 = 73.76 dB。
> 项目读两路信号：PA0（电位器，目标转速给定）和 PA1（INA240 电流输出），两路均 12-bit，输出范围 0~4095。
> PWM 占空比也是 12-bit（0~4095），整个系统精度统一在 12-bit 这一档。

> **[本项目 — 混叠风险]**: ADC 采样频率 = 1kHz（TIM6 中断周期），PA1 上的 INA240 输出包含 12.2kHz 的 PWM 纹波（FPGA 开关频率）。12.2kHz >> 500Hz（奈奎斯特），直接采样会产生严重混叠。
> 解决方式：软件 8 点移动平均滤波，等效于低通滤波，将纹波在数字域压低后再送入 PID。PA0 电位器在 ADC 引脚前接了 RC 低通滤波器，是模拟域的抗混叠滤波器（同时滤除电源噪声）。

---

## Page 2 - DAC Transfer Function

### Transfer function

- Graph: analog output $A$ versus digital input $D$.
- $D$ ranges from $0$ to $M-1$.
- $M=2^{n_b}$.
- Step size is 1 LSB.
- Vertical step marked as $\Delta A$.

### Static errors

#### Linear errors

Compare the **best linear approximation** with the **ideal transfer function**.

- **Offset**:

  $$
  \varepsilon_o=V_{off}
  $$

- **Gain**:

  $$
  \varepsilon_g=\frac{K'-K}{K}
  $$

  where the ideal transfer function is:

  $$
  A=K\cdot D
  $$

#### Non-linear errors

Compare the real transfer function with the best linear approximation.

- **Integral non-linearity**:
  - Maximum deviation along the vertical axis from the best linear approximation.

- **Differential non-linearity**:
  - Difference between two consecutive analog values.

  $$
  \varepsilon_{dnl}=A_d-A'_d
  $$

  - Non-monotonicity occurs when:

  $$
  \Delta_e<0
  \quad\text{or}\quad
  \varepsilon_{dnl}>1\,LSB
  $$

### Dynamic errors

- **Settling time** $T_{set}$:
  - Time for the DAC output to reach the new value.
  - Due to:
    - Slew rate of circuit.
    - Low-pass effects.
    - Oscillations.

- **Glitch**:
  - Caused by different switching delays.

---

## Page 3 - DAC Conversion

## Sum of quantities

### Uniform quantity

Each contribution is 1 LSB.

$$
A=D\cdot LSB
$$

- All voltage drops between steps:

  $$
  \frac{R}{(N-1)R}V_R=\frac{V_R}{N-1}
  $$

- Output range:

  $$
  V_{out}\in[0,V_R]
  $$

- Exponential number of resistors.
- All currents are identical:

  $$
  I_{elem}=\frac{V_R}{R}
  $$

- Output current range:

  $$
  I_{out}\in\left[0,(N-1)\frac{V_R}{R}\right]
  $$

- $N-1$ switches are controlled by thermometer output.

### Weighted quantity

$$
A=\sum_i\left(2^i d_i\right)\cdot LSB
$$

- $n_b$ switches.
- All currents are different:

  $$
  I_i=\frac{V_R}{2^iR},\qquad i\in[0,n_b-1]
  $$

- Output current:

  $$
  I_{out}\in\left[0,\frac{2V_R}{R}\right]
  $$

- $R_s$ should be small.
- $I_{out}$ and $R_{out}$ change with $D$.

Alternative structure:

- $I_s$ constant → easier to design.
- Smallest $R$ on MSB.
- Biggest $2^{n_b-1}R$ on LSB.

#### Voltage switches

- $0V \rightarrow V_R$.
- $I_s$ is not constant.
- $R_{out}$ is constant.
- $V_{out}\propto I_s$.
- Approximation:

  $$
  R_{out}\approx\frac{R}{2}
  $$

### Ladder network

- Linear number of resistors:

  $$
  2n_b+1
  $$

- Range of resistors: $R$ or $2R$.
- Constant $R_{out}$.

### Errors

- Error on $V_R$ → gain error.
- Leakage current of switches, non-zero when $D=0$ → offset.
- Offset of op-amp / transresistance amplifier → offset.
- Non-zero ON resistance of switches:
  - If constant:
    - Weighted mode: small error on LSBs.
    - Uniform mode: sums up with quantization.
  - If non-constant:
    - Non-linear errors.

- **Branch relative error** $B_{err}$ in weighted structure:
  - MSB weight is $1/2$.
  - MSB-1 weight is $1/4$.
  - Output error:

    $$
    E_{out}=B_{err}\cdot B_w
    $$

---

## Page 4 - ADC Conversion and ADC Circuits

### Continuation from DAC errors

- **Branch relative errors** $B_{err}$ in uniform structure:
  - Same error in different branches → same error.

- Switching: `[?]`

### ADC Conversion

#### Transfer function

- Staircase transfer function.
- Axes: $D$ versus $A$.
- $M=2^{n_b}$.
- Digital levels shown: $1,2,3,\ldots,M-1$.

#### Errors

- Similar to linear errors and non-linear errors in DACs.
- Compensation can be performed in the digital domain.
- Differential non-linearity on the $x$-axis can lead to **missing codes**.

#### Dynamic behavior

- Time required to have stable output.
- $CS \rightarrow EOC$ flag after $T_{conv}$.
- Sampling period condition:

  $$
  T_s>T_{conv}
  $$

  unless pipelining is used.

### ADC Circuits

#### Flash ADC

- Complexity:

  $$
  2^{n_b}-1
  $$

- Conversion time:

  $$
  1
  $$

- Timing condition:

  $$
  t_{conv}>t_{comp}+t_{encoder}
  $$

- Diagram content:
  - Reference ladder / resistor divider.
  - Multiple comparators.
  - Encoder converting comparator outputs to digital output.
  - Equivalent comparator representation with $V_R/2$.

#### Errors in flash ADC

- $V_R$ variation → gain error:

  $$
  \varepsilon_G=\frac{V_R-V'_R}{V_R}=\varepsilon_V
  $$

- Equal resistors, but values differ from ideal → no differential error in voltage divider.
- Different resistors → differential / transfer-function non-linearity.

---

## Page 5 - Feedback Converter, SAR, Residue, Parallel ADC

### Feedback converter

Timing condition:

$$
t_{clk}>t_{comp}+t_{logic}+t_{set}
$$

Diagram content:

- $V_{in}$ compared with feedback value.
- Up/down counter or logic block.
- DAC in feedback loop.
- Register / D flip-flop driven by clock.
- Output enable signal.

### Staircase converter

- Complexity:

  $$
  O(1)
  $$

- Conversion time:

  $$
  O(2^{n_b})
  $$

### Tracking converter

- Same as staircase, but no output register.
- One LSB at each clock:

  $$
  1LSB=\frac{S}{2^{n_b}}
  $$

- Tracking slew rate:

  $$
  SR_{track}=\frac{\Delta A}{t_{clk}}
  =\frac{S}{2^{n_b}t_{clk}}
  =\frac{S}{2^{n_b}}f_{clk}
  $$

- Sinusoidal-input slew rate condition noted:

  $$
  \frac{\Delta A}{t_{clk}}>SR_{sin}
  $$

  and approximately:

  $$
  f_{clk}>\frac{2^{n_b}\pi}{T_{in}}
  $$

- Complexity:

  $$
  O(1)
  $$

- Worst-case conversion time:

  $$
  O(2^{n_b})
  $$

### SAR converter

- Has output register with enable signal $\overline{EN}$.
- Complexity:

  $$
  O(1)
  $$

- Time:

  $$
  O(n_b)
  $$

> **[本项目]**: STM32F407 内部 ADC 就是 SAR 型。复杂度 O(1)（硬件面积小，适合集成在 MCU 里），转换时间 O(n_b) = O(12) 个 ADC 时钟周期。项目中 ADC 时钟 = APB2（84MHz）÷ 4 = 21MHz，每次转换约 12 + 采样周期个时钟，转换时间远小于 1ms 控制周期，满足 T_s > T_conv 条件。

---

### 为什么 MCU 片内 ADC 几乎都是 SAR 型

**最常用的 ADC 是 SAR 型**，原因是它在面积、速度、精度三个维度上对 MCU 来说是最优平衡：

| 类型 | 复杂度 | 转换时间 | 适用场景 |
|------|--------|----------|----------|
| Flash | O(2^n) | O(1) | 高速示波器，太大太贵 |
| Staircase | O(1) | O(2^n) | 太慢，不实用 |
| **SAR** | **O(1)** | **O(n)** | **MCU 片内，中速中精度** |
| Sigma-Delta | O(1) | 过采样，延迟大 | 音频、传感器，不适合实时控制 |

SAR 核心只需要：比较器 × 1 + DAC × 1 + 寄存器 × 1，面积极小，12-bit Flash 则需要 4095 个比较器。

### SAR 工作原理——本质是二分查找

12-bit SAR 做 12 次猜测，每次猜中间值，比较器告诉它"大了还是小了"，逐步逼近：

```
V_in 未知，量程 0~4095

第 1 次：DAC 输出 2048，比较器：V_in > 2048 → bit11 = 1，范围缩到 [2048, 4095]
第 2 次：DAC 输出 3072，比较器：V_in < 3072 → bit10 = 0，范围缩到 [2048, 3071]
第 3 次：DAC 输出 2560，比较器：V_in > 2560 → bit9  = 1，范围缩到 [2560, 3071]
...（共 12 次）
第12 次：最后一位确定，结果锁入输出寄存器
```

每次砍掉一半范围，12 次后精确到 1 LSB，所以时间是 O(n_b) 而不是 O(2^n_b)。

### SAR 内部结构图

```
        V_in
          │
          ▼
    ┌─────────────┐
    │  S/H 电路   │  ← 采样保持，在转换期间冻结输入
    └──────┬──────┘
           │ V_hold
           ▼
    ┌─────────────┐       ┌───────────────────┐
    │   比较器    │◄──────│  DAC（n_b bit）   │
    │  Comparator │       │  输出逐次逼近电压  │
    └──────┬──────┘       └────────┬──────────┘
           │ 1 bit result          │
           ▼                       │
    ┌──────────────────────────────┴──┐
    │     逐次逼近寄存器 SAR Logic    │
    │  每拍：result→存bit，更新DAC   │
    └──────────────────┬──────────────┘
                       │
                       ▼
              D[n_b-1 : 0]  输出
              （n_b 拍后有效）
```

数据流：V_hold 和 DAC 输出同时进比较器 → 比较结果告诉 SAR Logic 这一位是 0 还是 1 → SAR Logic 更新 DAC 输出为下一个猜测值 → 循环 n_b 次 → 结果寄存器输出。

### Residue converter

- SAR with $n_b$ iterations unrolled.
- Same time as SAR.
- More complex.
- Needs $n_b$ precision.
- Precision degrades toward the LSB.
- Diagrams show staged comparison and residue generation.

### Multibit residue converter

- Uses multiple bits per stage.
- Complexity:

  $$
  n_{stage}\cdot C_{stage}
  $$

  with:

  $$
  n_{stage}\approx\frac{n_b}{q_b}
  $$

  and $q_b$ bits per stage.

- Conversion time:

  $$
  n_{stage}\cdot t_{comp}+(n-1)t_{DAC}
  $$

- Normalized conversion time:

  $$
  n_{stage}\cdot t_{comp}
  $$

- Highlighted estimates:

  $$
  \text{Complexity}=O\left(\frac{n_b}{q_b}\cdot 2^{q_b}\right)
  $$

  $$
  \text{Conversion time}=O\left(\frac{n_b}{q_b}\right)
  $$

### Parallel AD

- $U$ conversion chains.
- $U$ times faster.
- Switch rate = sampling rate.
- Diagram content:
  - Interleaved sample-and-hold circuits.
  - ADCs feeding a multiplexer.

---

## Page 6 - Pipeline and Special ADCs

### Pipeline

- Need to reconstruct the full sample in the digital domain.

Delay of one stage:

$$
T_{stage}=t_{comp}+t_{DAC}+T_{S/H}\;[?]
$$

Total delay:

$$
T_{RES}=n_b\cdot T_{stage}
$$

Maximum conversion rate:

$$
f_{conv,pipe}=\frac{1}{T_{stage}}
$$

Conversion time:

- Latency:

  $$
  O(n_b)
  $$

- Pipeline throughput:

  $$
  O(1)
  $$

Complexity:

$$
C_{pipe}=O(n_b)\quad(+S/H)
$$

Diagram content:

- Sample-and-hold stage.
- 1-bit ADC.
- DAC feedback / residue generation.
- Repeated stages.
- Digital output bits such as $d_{MSB}$.

### Special ADCs

#### Delta $(\Delta)$ AD converter

- 1 bit.
- Serial.
- Integrator output jumps of $\gamma$.
- Related to tracking behavior.

Slew-rate condition:

$$
SR_\Delta>\frac{\gamma}{T_{clk}}
$$

Input / idle-noise condition:

$$
V_{min}>\frac{\gamma}{2}\quad\text{(idle noise)}
$$

Input signal limitation:

$$
SR_{max}<\frac{\gamma}{T_{clk}}
$$

For sinusoidal input, noted range:

$$
\frac{\gamma}{2}<V_p<\frac{\gamma}{\omega T_{clk}}
$$

- Need a small value of $\gamma$ for input-signal quantization:

  $$
  \gamma=\frac{S}{2^{n_b}}
  $$

- Need high clock frequency for sine input:

  $$
  f_{clk}>\frac{V_{p,max}\cdot\omega}{\gamma}
  $$

- Copies / codes are far apart `[?]`.

#### Sigma-Delta

- High speed.
- Non-weighted bitstream.
- Used for conversion on approximately 24-32 bits `[?]`.
- Diagram content:
  - Input summing node.
  - Integrator.
  - Comparator / quantizer.
  - Feedback loop.
  - D flip-flop / 1-bit output stream.

---

## Page 7 - Sample & Hold Circuits

### Sample & Hold circuits

Four phases are listed.

#### 1. Sample

- Aperture time: $t_a$.
- High-to-low transition.
- Transistor has some delay.
- Aperture-time jitter:

  $$
  t_a=t_a\pm t_j
  $$

- Error from finite aperture time:

  $$
  \Delta V_e=SR_{in}\cdot t_e
  $$

- Jitter error:

  $$
  \Delta V_j=SR_{in}\cdot t_j
  $$

- Jitter error cannot be compensated.
- If known downward `[?]`, it can be anticipated.

#### 2. Hold

- Stored value is kept on the output.
- Input changes have no effect.
- Low level of the control signal.
- At the end it switches low to high.
- Quantization happens here → $V_{hold}>t_{conv}$ `[?]`.
- $C_M$ keeps constant voltage, but there is a droop.

Droop:

$$
\frac{dV_{out}}{dt}=\frac{I_{droop}}{C_M}
$$

$$
\Delta V_{out}=\frac{I_{droop}\,t_{hold}}{C_M}
$$

#### 3. Acquisition

- Output changes to reflect the current value of the input.
- It takes some time to reach the input value.
- Occurs at the rising edge of the control signal plus part of the high level.
- Increases sampling period.
- Higher $C_M$ → higher $t_{acq}$.

#### 4. Track

- Output equals input / follows input.
- High level of control signal.

### Sample & Hold circuit diagrams

#### Simple switch-capacitor circuit

Diagram content:

- $V_{in}$ source.
- Source resistance $R_G$.
- Switch controlled by $V_Q$.
- Hold capacitor to ground.
- Load resistance and output $V_{out}$.

#### Op-amp sample-and-hold circuit

Diagram content:

- Input $V_{in}$.
- OA1 driving a switch $Q_1$.
- Holding capacitor $C_M$.
- OA2 configured as inverting stage.
- Load $R_L$ and output $V_{out}$.
- Feedback resistor noted as allowing different value.

Notes:

- Since OA2 is inverting.
- Feedback path allows different value `[?]`.

---

## Page 8 - Acquisition Chain and Channel Organization

### Signal chain

$$
\text{Protection}\rightarrow\text{Gain}\rightarrow\text{Anti-alias filter}\rightarrow S/H\rightarrow ADC\rightarrow DAC\rightarrow\text{Reconstruction filter}
$$

### Single channel

Conditions:

$$
f_s>2f_B
$$

$$
t_s>t_{acq}+t_{conv}
$$

Timing notes by converter type:

| Converter type | Timing note |
|---|---|
| SAR, RES | $n_b\cdot t_{clk}$ |
| Staircase | $2^{n_b}\cdot t_{clk}$ |
| Flash, Pipe | $t_{clk}$ |
| Tracking, Delta, Sigma-Delta | Oversampling; no need for S/H `[?]` |

### Multiple channels

Diagram content:

- Multiple inputs, e.g. $V_1$, $V_2$.
- Multiplexed switches sharing one output node $V_{out}$.
- Equivalent model includes a capacitor / parasitic capacitance and a source resistance $R_{source}$ `[?]`.

Overall chain:

$$
\text{Protection}\rightarrow\text{Gain}\rightarrow AA_f\rightarrow MUX\rightarrow S\&H\rightarrow ADC
$$

> **[本项目 — 采集链]**: ADC1 复用两路输入，对应课程的 Multiple channels 结构：
>
> **PA0 电位器通道**：
> 旋钮电位器 → RC 低通滤波（模拟域 AA 滤波） → PA0(IN0) → ADC1 MUX → S/H → 12-bit SAR → 软件 16 点均值滤波 → 目标转速给定
>
> **PA1 电流通道**：
> 电机电流 → R100 采样电阻（0.1Ω）→ INA240A2（增益 50x，对应课程的 Gain 环节）→ PA1(IN1) → ADC1 MUX → S/H → 12-bit SAR → 软件 8 点均值滤波（AA 滤波）→ 实际电流反馈(mA)
>
> 两路通道共用同一个 ADC1，在 1kHz 中断里顺序采集，MUX 切换由软件控制（写 ADC1->SQR3 寄存器选通道）。

