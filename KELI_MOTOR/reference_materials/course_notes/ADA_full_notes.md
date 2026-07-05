# ADA 手写笔记完整整理

> 来源文件：`ADA(1).pdf`，共 8 页手写笔记。本文按原页顺序整理，并尽量保留原公式、术语和图形信息。手写字迹较难辨认的地方用“原文不清/推断”标注；复杂电路图用 ASCII 图或文字说明复现。

---

## 目录

1. [Noise / 噪声](#1-noise--噪声)
2. [DAC Transfer Function / DAC 传输函数](#2-dac-transfer-function--dac-传输函数)
3. [DAC Conversion / DAC 转换结构](#3-dac-conversion--dac-转换结构)
4. [ADC Conversion / ADC 转换与误差](#4-adc-conversion--adc-转换与误差)
5. [ADC Circuits / ADC 电路结构](#5-adc-circuits--adc-电路结构)
6. [Pipeline ADC 与 Special ADCs](#6-pipeline-adc-与-special-adcs)
7. [Sample & Hold Circuits / 采样保持电路](#7-sample--hold-circuits--采样保持电路)
8. [系统级连接：单通道与多通道](#8-系统级连接单通道与多通道)

---

## 1. Noise / 噪声

### 1.1 Aliasing / 混叠

笔记中给出 aliasing 相关的信噪比表达。信号功率与混叠噪声功率之比可写为：

$$
SNR_e = \frac{\mu_{signal}}{N_{noise}}
$$

若前端滤波器为 $n_{poles}$ 阶，截止频率为 $f_c$，信号带宽上限为 $f_B$，采样频率为 $f_s$，则手写笔记中给出的形式为：

$$
SNR_e = \left(\frac{f_s - f_B}{f_c}\right)^{n_{poles}}
$$

对应的 dB 形式：

$$
SNR_e\big|_{dB}=20\,n_{poles}\,\log_{10}\left(\frac{f_s-f_B}{f_c}\right)
$$

### 1.2 Quantization / 量化噪声

量化噪声能量：

$$
E_{quant}=\frac{\Delta^2}{12}
$$

其中 $\Delta$ 是 1 LSB 的量化步长。

对不同输入信号，量化 SNR 的近似表达如下：

| 输入信号 | 量化 SNR 近似值 |
|---|---:|
| Triangular signal range $S$ | $SNR_q|_{dB}=6n$ |
| Sinusoidal signal range $S$ | $SNR_q|_{dB}=6n+1.76$ |
| Square signal range $S$ | $SNR_q|_{dB}=6n+4.77$ |
| Gaussian distribution | $SNR_q|_{dB}=6n-4.77$ |

> 这里 $n$ 表示量化位数。页面中将正弦输入的 $6n+1.76$ 作为重点标注。

### 1.3 Total SNR / 总信噪比

多个独立噪声源叠加时，线性域中：

$$
\frac{1}{SNR_{tot}}=\sum_i \frac{1}{SNR_i}
$$

对应 dB 域：

$$
SNR_{tot}=10\log_{10}\left(\frac{P_s}{P_{n,tot}}\right)
=-10\log_{10}\left(\sum_i 10^{-SNR_i/10}\right)
$$

### 1.4 ENOB / Effective Number of Bits

对 full-scale sine input：

$$
ENOB=\frac{SNR-1.76}{6}
$$

---

## 2. DAC Transfer Function / DAC 传输函数

### 2.1 理想 DAC 传输函数

手写图是一个阶梯状 DAC 传输函数。横轴是数字码 $D$，纵轴是模拟输出 $A$。

```text
A
^                         /  best linear approximation
|                      __/
|                   __/
|                __/
|             __/
|          __/
|       __/
|______/________________________________> D
  0    1    2       ...              M-1

M = 2^{n_b}
1 step = 1 LSB
```

其中：

$$
M=2^{n_b}
$$

### 2.2 Static Errors / 静态误差

静态误差分为 linear error 与 non-linear error。

#### Linear error

线性误差通过“best linear approximation”和 ideal transfer function 比较得到。

- **Offset error**：零点偏移。笔记写作：

  $$
  \varepsilon_o = V_{off}
  $$

  > 具体单位取决于横轴/纵轴归一化方式；原页只标出 $V_{off}$。

- **Gain error**：斜率误差。理想传输函数为：

  $$
  A=K\cdot D
  $$

  若实际斜率为 $K'$，则：

  $$
  \varepsilon_g=\frac{K'-K}{K}
  $$

#### Non-linear error

非线性误差通过 real transfer function 与 best linear approximation 比较得到。

- **Integral non-linearity, INL**：

  最大垂直偏差，即 real transfer function 到 best linear approximation 的最大 vertical-axis deviation。

  ```text
  A
  ^        real t.f.
  |       /_/--\__
  |      /       \     epsilon_inl: vertical deviation
  |-----/---------\---- best line
  +--------------------> D
  ```

- **Differential non-linearity, DNL**：

  相邻两个 analog values 之间的实际差值与理想差值的差：

  $$
  \varepsilon_{dnl}=\Delta A-A'_d
  $$

  笔记中写到：当以下情况出现时会造成 non-monotonicity：

  $$
  \Delta A \le 0
  \quad \text{or}\quad
  \varepsilon_{dnl}>1\,LSB
  $$

### 2.3 Dynamic Errors / 动态误差

- **Settling time**：DAC output 达到新 value 所需的时间。
  - 由 circuit 的 slew rate, SR 限制引起。
  - 由 low-pass effects 引起。
  - 由 oscillations 引起。

- **Glitch**：由不同 switches 的 switching delays 不一致导致。

---

## 3. DAC Conversion / DAC 转换结构

## 3.1 Sum of Uniform Q / 均匀单元求和 DAC

每个基本单元代表 1 LSB：

$$
A=D\cdot LSB
$$

### 电阻串分压形式

手写图为一串相同电阻，每个 tap 通过开关输出。

```text
V_R o--R--o--R--o--R--o-- ... --R--o
         |     |     |             |
        SW    SW    SW            SW
         |     |     |             |
       output selection by thermometer code
```

所有 voltage drops 相等。若共有 $N$ 个 levels，则每段电压：

$$
V_{elem}=\frac{R}{(N-1)R}V_R=\frac{V_R}{N-1}
$$

输出范围：

$$
V_{out}\in[0,V_R]
$$

### 电流源形式

所有 current sources are identical：

$$
I_{elem}=\frac{V_R}{R}
$$

输出电流范围：

$$
I_{out}\in\left[0,(N-1)\frac{V_R}{R}\right]
$$

需要 $N$ 个 switches，并由 thermometer output 控制。

```text
Thermometer code example:
D = 0   -> 00000000
D = 3   -> 11100000
D = 6   -> 11111100
```

## 3.2 Weighted Q / 二进制加权 DAC

笔记写到：

$$
A=\sum_i\left(2^i d_i\right)\cdot LSB
$$

其中 $d_i\in\{0,1\}$，$i\in[0,n_b-1]$。

### 电流加权结构

每一路电流不同：

$$
I_i=\frac{V_R}{2^iR}
$$

近似输出范围：

$$
I_{out}\simeq \left[0,\frac{2V_R}{R}\right]
$$

手写图说明：

- all currents different；
- output current $I_{out}$ 和 output resistance $R_{out}$ 会随 digital code $D$ 改变；
- $R_s$ should be small；
- $I_s$ constant 更容易设计；
- 最小电阻在 MSB；
- 最大电阻在 LSB：

$$
R_{LSB}=2^{n_b-1}R
$$

```text
Current weighted DAC, conceptual:

          I_s
          --->------------------o I_out
             |                  |
             R                  2^{n_b-1}R
             |                  |
            SW                 SW
             |                  |
            GND                GND

MSB branch: smallest R
LSB branch: biggest R
```

### Voltage switch 形式

笔记中还画出 voltage switches：每一路在 $0$ 与 $V_R$ 之间切换。

```text
               weighted resistors
V_R / 0  o--SW--R--+
V_R / 0  o--SW--2R-+---- output
V_R / 0  o--SW--4R-+
```

特点：

- $0V \to V_R$ switching；
- $I_s$ not constant；
- $R_{out}$ is constant；
- 近似：

$$
R_{out}\simeq \frac{R}{2}
$$

## 3.3 Ladder Network / R-2R 梯形网络

```text
R-2R ladder, conceptual:

        R        R        R
 o--R--o--R--o--R-- ... --o
       |        |          |
      2R       2R          2R
       |        |          |
      SW       SW         SW
       |        |          |
      0/V_R    0/V_R      0/V_R
```

笔记中的结论：

- 电阻数量线性增长：

  $$
  \#R=2n_b+1
  $$

- 电阻取值范围只有：

  $$
  R\quad \text{or}\quad 2R
  $$

- $R_{out}$ constant。

## 3.4 DAC Errors / DAC 误差来源

手写笔记列出的误差来源：

1. **Reference voltage 误差**

   $$
   \alpha V_R \rightarrow \text{gain error}
   $$

2. **Switch leakage current**

   - switches 的 leakage current；
   - 当 $D=0$ 时仍可能 non-zero；
   - 造成 offset。

3. **OpAmp offset**

   - transresistance 放大器中的 op amp offset；
   - 造成 offset。

4. **Switch on-resistance 非零**

   - switches 的 on resistance 不为零；
   - if constant：
     - weighted network 中对 LSBs 影响较小；
     - uniform network 中误差会累积；
   - if non-constant：会造成 non-linear errors。

5. **Branch relative error**

   在 weighted DAC 中，MSB 权重为 $1/2$，下一位为 $1/4$，依次递减。笔记写到：

   $$
   \varepsilon_{out}=B_{err}\cdot B_w
   $$

   > 这里 $B_{err}$ 和 $B_w$ 是原手写符号；含义可理解为 branch error 与 bit weight 的乘积。

6. **Uniform DAC 中的 branch relative errors**

   笔记写到：uniform 中相同 branch error 在不同 branch 上带来 same error。

7. **Switching**

   页面末尾单独写有 “Switching?”，应是提示开关瞬态也可能引入误差。

---

## 4. ADC Conversion / ADC 转换与误差

### 4.1 ADC Transfer Function

ADC transfer function 是 DAC transfer function 的反向：横轴为 analog input $A$，纵轴为 digital output $D$。

```text
D
^                         / ideal line
|                   _____/
|              ____/
|         ____/
|    ____/
|___/________________________________> A
  0    1    2    3      ...        M-1

M = 2^{n_b}
```

### 4.2 ADC Errors

笔记说明 ADC 的误差与 DAC 类似，但 ADC 中有些误差可在 digital domain 中补偿。

- Errors：same as DAC。
- 可在 Digital Domain 中 bit compensation。
- Differential non-linearity 出现在 x-axis 上，会造成：

  ```text
  missing code
  ```

### 4.3 ADC Dynamic Error

动态误差与转换时序有关：

- ADC 需要一定时间使 digital output 稳定。
- $CS \to EOC$ flag after $T_{conv}$。
- 通常要求：

  $$
  T_s>T_{conv}
  $$

- pipeline ADC 是例外，因为可以重叠转换。

---

## 5. ADC Circuits / ADC 电路结构

## 5.1 Flash ADC

Flash ADC 使用参考电阻串和大量比较器，然后通过 encoder 输出数字码。

```text
        V_R
         |
        R/2
         |----> comparator ----\
        R                      \
         |----> comparator ------> Encoder ---> D[n_b-1:0]
        R                      /
         |----> comparator ----/
        R/2
         |
        GND
```

等效比较器形式：

```text
Vin --- (+) comparator ----> logic
Vk/2 -- (-)
```

笔记给出的复杂度与时间：

- Complexity：

  $$
  2^{n_b}-1
  $$

- Conversion time：

  $$
  O(1)
  $$

- 实际转换时间需要满足：

  $$
  t_{conv}>t_{comp}+t_{encoder}
  $$

### Flash ADC errors

1. **Reference voltage error**

   $V_R$ 误差会造成 gain error。

   笔记中给出近似形式：

   $$
   \varepsilon_G=\frac{V_R-V_R'}{V_R'}
   $$

2. **Equal R 但实际值偏离 ideal**

   即所有电阻相等，但实际值不同于 ideal value。笔记说明会造成 voltage divider 中电流不同。

3. **Different $R_s$**

   不同电阻不匹配会造成不同 threshold/transfer function，从而产生 non-linearity。

---

## 5.2 Feedback Converter

反馈型转换器的基本结构：比较器比较 $V_{in}$ 和 DAC 反馈值 $V_A$；逻辑根据比较结果更新寄存器，再由 DAC 产生新的 $V_A$。

```text
          +------------------ logic / up-down / SAR ----------------+
          |                                                          |
Vin ---> comparator ---> control ---> Register D ----> DAC ----> V_A-+
```

时钟周期需大于比较、逻辑和稳定时间之和：

$$
t_{clk}>t_{comp}+t_{logic}+t_{stl}
$$

> 原文最后一项写作类似 $t_{stl}$，这里整理为 settling time。

### 5.2.1 Staircase ADC

Staircase converter 每个 clock 改变一次 DAC code，直到比较结果翻转。

```text
D: 0 -> 1 -> 2 -> 3 -> ... until V_DAC crosses V_in
```

笔记结论：

- Complexity：

  $$
  O(1)
  $$

- Conversion time：

  $$
  O(2^{n_b})
  $$

### 5.2.2 Tracking ADC

Tracking ADC 不是每次从 0 开始，而是从当前 code 继续向上/向下跟踪。

笔记内容：

- $D=D_A$；
- same but no output register；
- 每个 clock 变化 $1LSB$；

  $$
  \Delta A = \frac{S}{2^{n_b}}
  $$

- tracking slew rate：

  $$
  SR_{track}=\frac{\Delta A}{t_{clk}}
  =\frac{S}{2^{n_b}t_{clk}}
  =\frac{S}{2^{n_b}}f_{clk}
  $$

需要：

$$
SR_{track}>SR_{in}
$$

若输入为正弦，笔记写成近似形式：

$$
SR_{sin}\approx S\pi f_{in}
$$

所以 clock frequency 必须足够高，使 tracking 能跟上输入变化。

复杂度与最坏情况时间：

- Complexity：

  $$
  O(1)
  $$

- Worst-case conversion time：

  $$
  O(2^{n_b})
  $$

### 5.2.3 SAR ADC

Successive Approximation Register ADC 使用 SAR 寄存器逐位逼近。

```text
Vin -> comparator -> SAR -> DAC -> comparator feedback
                 ^    |
                 |    v
                EN / clk / output register
```

笔记结论：

- has output register with EN；
- Complexity：

  $$
  O(1)
  $$

- Time：

  $$
  O(n_b)
  $$

---

## 5.3 Residue ADC

Residue ADC 可以看作把 SAR 的 $n_b$ 次 iteration “unroll”。手写图中每一级都产生部分 bit，并对残差放大后送入下一阶段。

```text
Vin -> [stage 1: ADC/DAC/subtract/gain] -> residue -> [stage 2] -> ... -> bits
```

笔记内容：

- SAR $n_b$ iteration unroll；
- same time as SAR（原文如此写；结合后文 pipeline，可理解为未流水时延迟仍与级数相关）；
- more complex；
- need $n_b$ precision；
- precision degrades towards LSB。

### Multibit Residue

每级输出 $q_b$ bits。若每级 $q_b$ bits，则级数约为：

$$
N_{stage}=\frac{n_b}{q_b}
$$

手写笔记给出的复杂度与转换时间：

$$
Complexity=O\left(\frac{n_b}{q_b}2^{q_b}\right)
$$

$$
Conversion\ time=O\left(\frac{n_b}{q_b}\right)
$$

---

## 5.4 Parallel ADC

Parallel AD 使用 $U$ 条 conversion chains 并行工作。

```text
          +-- S/H -- ADC --+
Vin ---+--+-- S/H -- ADC --+--> MUX --> digital output
       +--+-- S/H -- ADC --+
```

笔记结论：

- $U$ conversion chains；
- $U$ times faster；
- Switch Rate = Sampling Rate。

---

## 6. Pipeline ADC 与 Special ADCs

## 6.1 Pipeline ADC

Pipeline ADC 对 residue stage 加流水线，使每一级可同时处理不同 sample。

```text
Vin -> S/H -> [1 ADC] -> digital bits d_MSB
              |   ^
              v   |
             [1 DAC]
              |
          subtract / gain -> next S/H -> next stage
```

笔记内容：

- need to reconstruct full sample in digital domain；
- 每一级需要 ADC/DAC/残差处理；
- stage delay：

  $$
  T_{stage}=t_{comp}+t_{DAC}\quad(\text{原文旁注含 }T_{S/H})
  $$

- total delay：

  $$
  T_{res}=n_b\,T_{stage}
  $$

- maximum conversion rate：

  $$
  f_{conv,pipe}=\frac{1}{T_{stage}}
  $$

- conversion time：
  - Latency：

    $$
    O(n_b)
    $$

  - Pipeline throughput：

    $$
    O(1)
    $$

- Complexity：

  $$
  C_{pipe}=O(n_b)\quad(+S/H)
  $$

---

## 6.2 Special ADCs

### 6.2.1 Delta ADC

Delta AD converter 使用 1-bit serial 反馈结构。

```text
Vin ---> (+) comparator ---> switch / bitstream ---> integrator / LPF ---> Vout
          ^                                             |
          |                                             |
          +---------------- feedback -------------------+
```

笔记内容：

- DELTA $(\Delta)$ AD converter；
- 1 bit；
- serial；
- integrator output jumps of $\gamma$；
- $\Delta$ in tracking。

斜率限制：

$$
SR_{\Delta}=\frac{\gamma}{T_{clk}}
$$

需要输入信号的 slew rate 小于可跟踪斜率：

$$
SR_{max}=\frac{\gamma}{T_{clk}}
$$

手写笔记中还写有输入信号幅度范围的限制：

$$
\frac{\gamma}{2}<V_p<\frac{\gamma}{\omega T_{clk}}
$$

以及：

- need small value of $\gamma$，因为 $\gamma$ 是 input signal quantization step；
- 笔记写：

  $$
  \gamma=\frac{S}{2^{n_b}}
  $$

- 对 sine input，需要高 clock frequency：

  $$
  f_{clk}>\frac{V_{peak}\omega}{\gamma}
  $$

- copies are far apart（原文写法；可理解为高采样率下频谱镜像远离信号带）。

### 6.2.2 Sigma-Delta ADC

Sigma-Delta 结构在 delta 思想上加入积分/噪声整形。笔记画的是一阶 sigma-delta loop：

```text
Vin ---> (+) ----> integrator ----> 1-bit quantizer ----> bitstream ----> LPF/decimator ---> Vout
          ^                               |
          |                               |
          +------------ 1-bit DAC --------+
```

手写笔记内容：

- high speed, non-weighted bitstream；
- for conversion on 24-32 bits；
- 图中下方给出开关电容实现：输入经过电阻/电容进入运放积分器，再进入比较器/D 触发器，输出回馈到输入端。

---

## 7. Sample & Hold Circuits / 采样保持电路

## 7.1 Four Phases / 四个阶段

手写笔记列出 Sample & Hold 的 4 phases：

1. Sample
2. Hold
3. Acquisition
4. Track

### 7.1.1 Sample phase

Sample 阶段涉及 aperture time：

- Aperture time：$t_a$；
- high-to-low transition；
- transistor has some delay；
- aperture jitter：

  $$
  t_a=t_a\pm t_j
  $$

由 jitter 引起的电压误差：

$$
\Delta V_j=SR_{in}\,t_j
$$

若控制 command 已知，可以提前补偿一部分固定延迟；但 jitter 本身 cannot be compensated。

### 7.1.2 Hold phase

Hold 阶段的定义：

- stored value is kept on the output；
- input changes have no effects；
- low level of the control signal；
- at the end, control signal makes low-to-high transition；
- quantization level 约束：原文写作类似 $V_{hold}>t_{conv}$，应理解为 hold 时间需覆盖转换时间。

保持电容 $C_m$ 维持电压，但存在 droop：

$$
\frac{dV_{out}}{dt}=\frac{I_{drop}}{C_m}
$$

经过时间 $t$ 后：

$$
\Delta V_{out}=\frac{I_{drop}t}{C_m}
$$

若保持时间主要为 ADC conversion time：

$$
\Delta V_q=\frac{I_{drop}t_{conv}}{C_m}
$$

### 7.1.3 Acquisition phase

Acquisition 阶段：output changes to reflect the current value of input。

笔记要点：

- 需要一定时间才能 reach input value；
- 发生在 control signal rising edge 以及 high level 的一部分；
- increases sampling period；
- higher $C_m$ 会导致 higher $t_{acq}$。

### 7.1.4 Track phase

Track 阶段：

- output = input，输出 following 输入；
- high level of control signal。

---

## 7.2 Sample & Hold Circuits / 采样保持电路图

### 7.2.1 基本开关电容 S/H

手写图为输入通过开关给保持电容充电，输出端有负载电阻。

```text
             R_G          Q switch
U_in o------/\/\/\------o/ o------o------ Vout
                         |        |
                         |       C_m
                         |        |
                        V_G      GND
                                  |
                                 R_L
                                  |
                                 GND
```

说明：

- $V_G$ 控制开关 $Q$；
- sample/track 时开关闭合，$C_m$ 跟随输入；
- hold 时开关断开，$C_m$ 保存电压；
- 负载 $R_L$ 和漏电流会造成 droop。

### 7.2.2 运放缓冲 S/H

原图下半部分是带两个运放的采样保持电路：

```text
Vin --> OA1 buffer --> Q switch --> C_m --> OA2 buffer/inverter --> R_L --> Vout
           ^                              |
           |                              |
           +----------- R_p feedback -----+
```

手写标注：

- Since OA2 inverting；
- feedback path 用于 allow different value（原文较模糊，可理解为通过反馈/增益设置允许不同输出比例）；
- $C_m$ 是保持电容；
- $R_L$ 是输出负载。

---

## 8. 系统级连接：单通道与多通道

## 8.1 总体信号链

页面顶端给出从模拟输入到数字处理、再到模拟输出的链路：

```text
Input
  |
  v
Protection -> Gain -> Anti-Alias Filter -> S/H -> ADC -> {digital domain} -> DAC -> Reconstruction Filter
```

其中：

- Protection：输入保护；
- Gain：增益调节；
- Anti-Alias Filter：ADC 前的抗混叠滤波；
- S/H：采样保持；
- ADC：模数转换；
- DAC：数模转换；
- Reconstruction Filter：DAC 后重构滤波。

## 8.2 Single Channel / 单通道

单通道条件：

$$
f_s>2f_B
$$

采样周期必须大于 acquisition time 和 conversion time 之和：

$$
t_s>t_{acq}+t_{conv}
$$

不同 ADC 结构的转换时间：

| 结构 | 转换时间 |
|---|---:|
| SAR, RES | $n_b\,t_{clk}$ |
| Staircase | $2^{n_b}\,t_{clk}$ |
| Flash, Pipeline | $t_{clk}$ |
| Tracking, Delta, Sigma-Delta | oversampling，通常 no need for S/H |

> 原页将 Tracking/Delta/Sigma-Delta 归在 oversampling 类，并标注 “No need for S/H”。

## 8.3 Multiple Channels / 多通道

页面中画出多通道输入共享节点的寄生电容、泄漏和源阻抗模型。

```text
V1 o----||----+
             |
V2 o----||----+---- Vout
             |
            parasitic / shared node
```

另一处小图给出 leakage 与 source resistance：

```text
      I_leak
        |
input --+----/\/\/\---- output
             R_source
```

多通道系统常见结构：

```text
Channel 1 -- Protection/Gain/AA filter --+
Channel 2 -- Protection/Gain/AA filter --+--> MUX --> S/H --> ADC
Channel N -- Protection/Gain/AA filter --+
```

手写图的最后一行：

```text
Prot -> Gain -> AA f -> MUX -> S&H -> ADC
```

含义：

- 每个通道前端可能需要 protection、gain 和 anti-alias filter；
- 多路信号通过 MUX 选择；
- 被选中信号进入 S/H，再进入 ADC；
- 多通道时要考虑串扰、寄生电容、源阻抗、漏电流以及通道切换后的 acquisition time。

---

# 附：缩写表

| 缩写 | 含义 |
|---|---|
| ADC | Analog-to-Digital Converter |
| DAC | Digital-to-Analog Converter |
| S/H | Sample and Hold |
| LSB | Least Significant Bit / 最低有效位对应步长 |
| MSB | Most Significant Bit / 最高有效位 |
| SNR | Signal-to-Noise Ratio |
| ENOB | Effective Number of Bits |
| INL | Integral Non-Linearity |
| DNL | Differential Non-Linearity |
| SAR | Successive Approximation Register |
| EOC | End of Conversion |
| MUX | Multiplexer |
| LPF | Low-Pass Filter |

---

# 附：无法完全确认的手写项

为保持完整性，以下项目已按上下文整理，但原手写字迹存在不确定性：

1. Page 2 中 offset error 的公式只清楚写到 $V_{off}$，归一化因子未明确。
2. Page 3 中 weighted DAC 的部分电流范围和 $R_s$ 注释较模糊，已按常见二进制加权 DAC 解释整理。
3. Page 3 的 branch relative error 公式中 $B_{err}$、$B_w$ 的具体变量定义未在原页给出。
4. Page 4 中 Flash ADC 的某些 resistor mismatch 说明较简略，已整理为 reference、divider current 和 threshold non-linearity 三类误差。
5. Page 5 的 Tracking ADC 正弦输入 clock 条件原文较难辨认，已保留关键条件 $SR_{track}>SR_{in}$。
6. Page 7 的 hold phase 中 “quantization level” 旁的式子字迹不清，已按 hold time 覆盖 conversion time 的含义整理。
