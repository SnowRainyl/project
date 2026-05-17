# Power Management

> Source: `Power management .pdf`  
> Notes: This Markdown is a page-by-page transcription of the handwritten notes. Circuit drawings and graphs are preserved as page images; key equations and written annotations are typed below each page. Unclear handwriting is marked as `[unclear]`.

---

## Page 1 - Power Devices

![Page 1](assets/page_01.png)

### Diodes

- Forward voltage:
  - $V_f \approx 0.6-0.7\,\text{V}$
- ON resistance:
  - $R_{on}=\dfrac{\Delta V_D}{\Delta i_D}$
- Diode equation:

$$
i_D(t)=I_S\left(e^{\frac{v_D(t)}{\eta V_T}}-1\right)
$$

- Zener diode:
  - Breakdown voltage: $V_{BR}$
- Reverse recovery time:
  - $t_{rr}$: when going from forward bias to switch-off.
  - Carriers must be removed.
  - $t_{rr}=t_s+t_{tr}$
  - $t_s$: storage time
  - $t_{tr}$: reverse transition time
- Forward recovery time:
  - $t_{fr}$: no stored charge to remove.

### Power diodes

For power applications: **P-I-N** structure.

- Increased reverse breakdown voltage:
  - $V_{BR}$ increased, typically hundreds of volts.
- $R_{on}$ and $V_f$ increase.

### Schottky diode

- Metal-semiconductor junction, usually metal-silicon.
- Fast switching in both directions.
- Lower $R_{on}$ and lower $V_\gamma$.
- Higher capacitance in OFF state.
- Higher reverse current $I_S$.
- Lower $V_{BR}$ in absolute value.

### BJT - Bipolar Junction Transistor

- Terminals: base, collector, emitter.
- Cut-off:
  - BE and BC junctions reverse-biased.
  - $i_C \approx 0$.
- Forward active:
  - BE forward-biased, BC reverse-biased.
  - Acts as a linear amplifier.
- Saturation:
  - BE and BC forward-biased.
  - Closed switch behavior.
  - $V_{CE}\approx 0$.

Parameters:

- $\beta$: current gain in forward active region.

$$
i_C=\beta i_B
$$

- $V_{BE,on}$: minimum voltage to turn on the transistor.
- $V_{CE,sat}\approx 0.2-0.3\,\text{V}$.
- $V_{BE,sat}\approx 1\,\text{V}$ when transistor is saturated.
- Reverse recovery problem in BJT:
  - Base-collector junction stores charge.
  - Need to remove charge when switching off.

---

## Page 2 - MOS Transistor and Low-Side NPN BJT Drive

![Page 2](assets/page_02.png)

### MOS transistor

MOS = Metal-Oxide-Semiconductor.

Regions shown in $i_D$ vs. $V_{DS}$ characteristic:

- Cut-off
- Triode / linear region
- Saturation

MOSFET symbol annotations:

- Gate: $G$
- Drain: $D$
- Source: $S$
- $V_{GS}$, $V_{DS}$, $i_D$

Parameters and operating regions:

- $R_{DS,on}$:
  - inverse slope of linear region.
  - For low $\Omega$, use large $V_{GS}$.
- Threshold voltage:
  - $V_{th}$: minimum voltage to turn ON.
  - Typical value: $\sim 1-1.5\,\text{V}$.
- Cut-off:

$$
V_{GS}<V_{th}\Rightarrow i_D=0
$$

- Saturation:

$$
V_{GS}>V_{th},\quad V_{DS}>V_{GS}-V_{th}
$$

  - Linear amplifier behavior.

- Triode:

$$
V_{GS}>V_{th},\quad V_{DS}<V_{GS}-V_{th}
$$

  - Closed switch behavior.
  - $r_{DS}\approx 0$.

Maximum ratings:

- Maximum drain-source voltage: $BV_{DSS}$
- Maximum drain current: $I_{D,max}$
- Maximum power dissipation:

$$
P_{D,max}=I_DV_{DS}
$$

For power applications: **DMOS**.

> **[本项目]**: TB6612FNG 内部使用 DMOS 工艺的 N 沟道 MOSFET 构成 H 桥。电机驱动时 MOSFET 工作在 **triode（线性）区**（相当于闭合开关，$R_{DS,on}$ 极小），而非 saturation 区。TB6612 单路 $R_{DS,on}$(高侧+低侧合计) 约 $0.5\,\Omega$，持续电流 1.2A，峰值 3.2A。

### Actuator driving: low-side NPN BJT

Circuit: load connected to $V_L>V_{DD}$, NPN transistor on low side, driven by digital logic through $R_B$.

#### Transistor OFF

- Force:

$$
V_{BE}<V_{BE,on}
$$

- Usually:

$$
V_{OL}<V_{BE,on}
$$

#### Transistor ON

- Collector current:

$$
I_C=\beta I_B
$$

- To force saturation:

$$
I_B \gg \frac{I_C}{\beta}
$$

- Forced beta:

$$
\beta_f=\frac{I_C}{I_B}
$$

- Saturation voltages:

$$
V_{CE}=V_{CE,sat}\approx 0.2-0.3\,\text{V}
$$

$$
V_{BE}=V_{BE,sat}\approx 0.8-1\,\text{V}
$$

- Base current depends on $V_{OH}$:

$$
I_B=\frac{V_{OH}-V_{BE,sat}}{R_B}
$$

To force saturation:

$$
I_B\gg \frac{I_C}{\beta}
$$

Thus:

$$
\frac{V_{OH}-V_{BE,sat}}{R_B}\gg \frac{I_C}{\beta}
$$

Upper bound:

$$
R_B \ll \frac{\beta}{I_C}\left(V_{OH}-V_{BE,sat}\right)
$$

Base current must not exceed the maximum $I_{OH}$ of the digital output:

$$
\frac{V_{OH}-V_{BE,sat}}{R_B}<I_{OH}
$$

Lower bound:

$$
R_B>\frac{V_{OH}-V_{BE,sat}}{I_{OH}}
$$

Current when the transistor is saturated:

$$
I_C\approx \frac{V_L-V_{CE,sat}}{R_L}
$$

### Higher current requirement: Darlington transistor

Darlington pair increases current gain:

$$
I_{B2}=\beta_1 I_{B1}
$$

$$
I_{C2}=\beta_2 I_{B2}=\beta_2\beta_1I_{B1}
$$

Equivalent current gain:

$$
\beta_{eq}=\frac{I_{C2}}{I_{B1}}=\beta_2\beta_1
$$

A driver can also use parallel digital outputs, but this may introduce delays.

---

## Page 3 - Dynamic BJT Behavior, Inductive Load, Low-Side nMOS

![Page 3](assets/page_03.png)

### Dynamic behavior of BJT drive

- Reverse recovery / storage:

$$
t_{rr}=t_s+t_{tr}
$$

- Schottky clamp can be used to avoid saturation of the base-collector junction.
  - Avoids stored charge.
  - No charge to remove during switch-off.

- Speed-up techniques:
  - Increase base current during transient.
  - During transient $T$: capacitor acts like short circuit.
  - After transient: capacitor acts open.
  - A capacitor can be placed in parallel with $R_B$.

### If the driver output is open collector

Use pull-up resistor $R_{PU}$.

When output is low:

$$
\frac{V_L}{R_{PU}}<I_{OL}
$$

Therefore:

$$
R_{PU}>\frac{V_L}{I_{OL}}
$$

This limits current when the output is low.

When output is high:

- $R_{PU}$ is in series with $R_B$.
- $R_{PU}$ should be low enough to force saturation:

$$
R_{PU}+R_B \ll \frac{\beta}{I_C}\left(V_L-V_{BE,sat}\right)
$$

### Inductive load

- Flywheel diode required.
- Inductor voltage:

$$
v(t)=L\frac{di(t)}{dt}
$$

> **[本项目]**: JGA25-370 直流电机是典型感性负载（绕组电感约数 mH）。PWM 关断瞬间电感产生反向尖峰电压。**TB6612FNG 内部集成了续流二极管（flywheel diode）**，无需外接，自动钳位反向尖峰，保护 MOSFET。这也是选用集成驱动芯片而非分立 MOSFET 的原因之一。

### Low-side nMOS

#### Transistor OFF

$$
V_{GS}<V_{th}
$$

Digital output low:

$$
V_{OL}\text{ is fine }(<1\,\text{V})
$$

#### Transistor ON

- Raise $V_{GS}$ high enough:

$$
V_{GS}\gg V_{th}
$$

- To bring MOSFET into triode region:

$$
V_{OV}=V_{GS}-V_{th}
$$

- Higher $V_{OV}$ gives lower $V_{DS}$.
- The device is more strongly in triode.

Transfer characteristic:

- Linear / triode and saturation regions.
- In all cases:

$$
V_{GS}>V_{th}
$$

Linear approximation:

$$
i_{DS}=\mu C_{ox}\frac{W}{L}(V_{GS}-V_{th})V_{DS}
$$

$$
R_{DS,on}=\frac{V_{DS}}{i_{DS}}=
\frac{1}{\mu C_{ox}\frac{W}{L}(V_{GS}-V_{th})}
$$

Triode region:

$$
i_{DS}=\mu C_{ox}\frac{W}{L}\left[(V_{GS}-V_{th})V_{DS}-\frac{V_{DS}^2}{2}\right]
$$

Saturation region:

$$
i_{DS}=\frac{1}{2}\mu C_{ox}\frac{W}{L}(V_{GS}-V_{th})^2
$$

Region conditions:

- Triode:

$$
V_{DS}<V_{GS}-V_{th}
$$

- Saturation:

$$
V_{DS}>V_{GS}-V_{th}
$$

Load-line note:

- Intersect load line and characteristic curve.
- Check that $V_{DS}<V_{GS}-V_{th}$ for triode operation.
- For linear/resistive load, the load line is equivalent to a resistor load.

### Dynamic behavior of MOSFET

- Gate capacitances:
  - $C_{GS}$
  - $C_{GD}$
  - $C_{DS}$
- $C_{GD}$ is amplified by Miller effect.
- Drive MOS with low output resistance and sufficient current.

---

## Page 4 - High-Side Driving

![Page 4](assets/page_04.png)

### High-side driving: NPN BJT

Problem:

- NPN cannot be switched ON normally in high-side configuration.

Solutions:

- Charge pump
- Level shifter

### High-side driving: PNP BJT

- PNP often has lower $\beta$.
- Digital output high can be used to keep the transistor OFF.
- Operation is opposite of NPN.

Two cases:

#### Case 1: $V_{DD}=V_L$

- Same considerations as low-side BJT, but with opposite signs.

#### Case 2: $V_{DD}<V_L$

- Low logic output switches the transistor ON.
- Transistor may not be switchable OFF directly because the base-emitter junction can remain forward-biased:

$$
V_{BE}=V_{CC}-V_L
$$

in absolute value.

Solutions:

- Use open collector and $R_{PU}$ to $V_L$.
- Use a low-side NPN driver transistor $T_2$.
  - $T_1$ handles the high voltage/current side.
  - $T_2$ handles lower current.

Logic behavior with NPN pre-driver:

- Logic low:
  - Low-side NPN $T_2$ is OFF.
  - High-side PNP $T_1$ is also OFF.
- Logic high:
  - Low-side NPN $T_2$ saturates.
  - Current flows in the base of PNP $T_1$.
  - PNP $T_1$ saturates.

Design inequalities shown:

$$
I_{B1}\gg \frac{I_{C1}}{\beta_1}
$$

$$
I_{B2}\gg \frac{I_{C2}}{\beta_2}=\frac{I_{B1}}{\beta_2}
$$

$$
R_{B2}\ll \frac{\beta_1\beta_2}{I_{C1}}(V_{OH}+V_{BE2,sat})
$$

Digital output current limit:

$$
R_{B2}>\frac{V_{OH}-V_{BE2,sat}}{I_{OH}}
$$

or, depending on the supply used:

$$
R_{B2}>\frac{V_{DD}-V_{BE2,sat}}{I_{OH}}
$$

Pull-up sizing note:

$$
R_{PU}\approx \frac{V_{BE1,sat}}{I_{C1}/\beta_1}
$$

### High-side nMOS

- Difficult to turn ON.
- Requires:
  - Charge pump
  - Level shifter
- nMOS has lower $R_{DS,on}$ than pMOS.
- Conditions discussed:
  - When load voltage goes to $V_{DD}$.
  - When load voltage is above $V_{DD}$.

---

## Page 5 - High-Side pMOS and H Bridges

![Page 5](assets/page_05.png)

### High-side pMOS

- Complementary to nMOS low-side driver.

Use case:

- When load voltage is higher than what the digital circuit can sustain, use a low-side nMOS pre-driver.

Notes:

- $M_2$ has the same voltage stress as $M_1$, but much lower current.
- Pull-up $R_{PU}$ high switches OFF $M_1$ when $M_2$ is OFF.

### Floating load

#### Half H bridge

- Some loads need current in both directions.
- nMOS implementation shown.
- pMOS may require a low-side nMOS pre-driver if $V_L>V_{DD}$.
- Logic block with enable and direction signals shown.

#### Full H bridge

- Built from two half bridges.
- Transistors always switch ON in opposite pairs.
- Load can be left floating:

$$
\text{all transistors OFF}
$$

> **[本项目]**: TB6612FNG 就是一块完整的 Full H bridge（实际是双路，本项目用 A 路）。方向控制由 AIN1/AIN2 决定对角开关组合（AIN1=1,AIN2=0 → 正转；AIN1=0,AIN2=1 → 反转；AIN1=AIN2=0 → 滑行/制动）。本项目固定 AIN1=3.3V、AIN2=GND，即固定正转，速度仅由 PWMA 的占空比控制。**STBY 引脚必须拉高**才能使能 H 桥；STBY=0 时所有开关关断，等效于 all transistors OFF（本项目接 3.3V 固定使能）。

---

## Page 6 - Floating Switch and Power Supply Basics

![Page 6](assets/page_06.png)

### Floating switch nMOS

A floating nMOS switch topology is shown.

### Power supply

#### Load regulation

$$
S_I=R_{out}=\frac{\Delta U_{out}}{\Delta I_{out}}
$$

Ideally:

$$
S_I=0
$$

#### Line regulation

$$
S_V=\frac{\Delta V_{out}}{\Delta V_{in}}
$$

Ideally:

$$
S_V=0
$$

#### Efficiency

$$
\eta=\frac{P_L}{P_{tot}}
$$

Comparison:

- Linear regulator:
  - Low efficiency.
  - Excellent regulation / sensitivity.
- Switching regulator:
  - High efficiency.
  - Good regulation / sensitivity.

> **[本项目（板载供电）]**: STM32F407G-DISC1 板载一个 **LDO 线性稳压器**（LD39150，5V→3.3V），为 MCU 核心、GPIO、INA240 模块、OLED、TB6612 逻辑侧（VCC）统一供电。LDO 效率 $\eta \approx 3.3/5 = 66\%$，多余的 $\sim 33\%$ 以热量耗散在芯片内部——这是 Linear regulator 低效的典型例子。电机主电源（VM，5~15V）直接来自外部电源，不经过板载 LDO。

> **[本项目（PWM 效率）]**: 这正是为什么电机调速用 **PWM + H 桥（开关型）**而不是用线性三极管调压。如果用线性方式把 12V 降到 6V（50% 速度），效率只有 $\eta \approx V_{out}/V_{in} = 50\%$，其余全变成热量耗散在三极管上。而 PWM 开关方式理论效率接近 100%，功耗主要来自 MOSFET 的 $R_{DS,on}$ 和开关损耗（12.2kHz 时很小）。本项目电机额定电流约 200~300mA，线性方案会在驱动管上白白散掉数瓦，PWM 方案几乎不发热。

Power supply chain shown:

```text
switch/fuse -> filter -> transformer -> rectifier -> filter -> regulator -> Vout
```

---

## Page 7 - Rectifier and Filter

![Page 7](assets/page_07.png)

## Rectifier

### Half wave

- Single diode rectifier.
- Diode drop:

$$
V_D\approx 1\,\text{V}
$$

### Full wave

Two configurations shown:

1. Center-tapped transformer with two diodes.
2. Bridge rectifier with four diodes.

Notes:

- Diodes work separately:
  - voltage drop $\approx V_D$.
  - current is divided, giving lower power dissipation.
- Bridge rectifier:
  - $2V_D$ voltage drop.
  - two diodes conduct at a time.
  - higher consumption.
  - normal transformer is simpler.

Design notes:

- Maximum reverse voltage must be considered.
- Maximum current must be considered.

## Filter

Purpose:

- Convert pulsating output of rectifier into pseudo-DC.

Simplest filter:

- Capacitor in parallel with the load.

Peak voltage:

$$
V_{max}=V_A-V_D
$$

or, for four-diode bridge:

$$
V_{max}=V_A-2V_D
$$

Ripple from capacitor discharge:

$$
\Delta V_{out}(t)=\frac{I_{load}t}{C}
$$

For full-wave rectification:

$$
\Delta V=\frac{I_{load}}{C}\frac{T}{2}
$$

Minimum voltage:

$$
V_{min}=V_{max}-\Delta V
$$

Average voltage:

$$
V_{avg}=\frac{V_{max}+V_{min}}{2}=V_{max}-\frac{\Delta V}{2}
$$

For half rectifier:

$$
\Delta V=\frac{I_{load}}{C}T
$$

- Larger conduction time.
- Higher peak current.

---

## Page 8 - Reverse Voltage, Peak Current, Linear Regulator

![Page 8](assets/page_08.png)

### Maximum reverse voltage

When a diode is not conducting:

- $V_{max}\approx V_A$.
- Input voltage can go to minimum $-V_A$.
- Total reverse voltage is approximately $2V_A$.
- $V_A$ is the peak voltage at the output of the transformer.

Diode choice:

- For half-wave and full-wave with two diodes:

$$
V_{BR}>2V_A
$$

- For full-wave bridge with four diodes:

$$
V_{BR}>V_A
$$

because two diodes are in series.

### Current peak

Input waveform:

$$
V_{in}=V_{max}\cos\left(\frac{2\pi}{T}t\right)
$$

Conduction time base:

$$
t_c=\frac{T}{2\pi}\cos^{-1}\left(\frac{V_{max}-\Delta V}{V_{max}}\right)
$$

- The longer $t_c$, the higher the residual ripple $\Delta V$.

Half-wave:

$$
\frac{t_c I_{D,peak}}{2}=TI_{out}
$$

$$
I_{D,peak}=2I_{out}\frac{T}{t_c}
$$

Full-wave:

$$
\frac{t_c I_{D,peak}}{2}=\frac{T}{2}I_{out}
$$

$$
I_{D,peak}=I_{out}\frac{T}{t_c}
$$

Higher capacitance:

$$
C\uparrow \Rightarrow \text{ripple}\downarrow
$$

## Linear regulator

Basic relations:

$$
I_{in}=I_R+I_{out}
$$

Typically:

$$
I_R\ll I_{out}
$$

Dropout condition:

$$
V_D\ge V_{D,min}
$$

Efficiency:

$$
\eta=\frac{P_{out}}{P_{in}}
=\frac{V_oI_o}{V_iI_i}\approx\frac{V_{out}}{V_{in}}
$$

Using average values:

$$
\eta\approx\frac{V_o}{V_s}
$$

and approximately:

$$
\eta_{max}=1-\frac{\Delta V/2+V_{D,min}}{V_{avg}}
$$

---

## Page 9 - Zener Linear Regulator

![Page 9](assets/page_09.png)

# Zener linear regulator

Circuit: supply $V_S$, series resistor $R_S$, Zener diode modeled with $V_{Z0}$ and dynamic resistance $r_Z$, load current $I_o$, load voltage $V_L$.

Output voltage:

$$
V_L=rac{\frac{V_S}{R_S}+\frac{V_{Z0}}{r_Z}-I_o}{\frac{1}{R_S}+\frac{1}{r_Z}}
$$

### Minimum Zener current

Worst case:

$$
V_S=V_{S,min}=V_{avg}-\frac{\Delta V}{2}
$$

$$
I_o=I_{o,max}
$$

Condition:

$$
I_Z=\frac{V_L'-V_{Z0}}{r_Z}>I_{Z,min}
$$

where:

$$
V_L'=\frac{\frac{V_{S,min}}{R_S}+\frac{V_{Z0}}{r_Z}-I_{o,max}}{\frac{1}{R_S}+\frac{1}{r_Z}}
$$

Therefore:

$$
R_S<R_{S,max}=\frac{V_{S,min}-V_{Z0}-r_ZI_{Z,min}}{I_{o,max}+I_{Z,min}}
$$

### Maximum Zener power

Worst case:

$$
V_S=V_{S,max}=V_{avg}+\frac{\Delta V}{2}
$$

$$
I_o=I_{o,min}\quad\text{or worst case }0\,\text{A}
$$

Power condition:

$$
I_ZV_L''<P_{Z,max}
$$

with:

$$
I_Z=\frac{V_L''-V_{Z0}}{r_Z}
$$

$$
V_L''=\frac{\frac{V_{S,max}}{R_S}+\frac{V_{Z0}}{r_Z}}{\frac{1}{R_S}+\frac{1}{r_Z}}
$$

Thus:

$$
R_S>R_{S,min}
$$

### Load regulation

$$
S_I=\frac{\Delta V_L}{\Delta I_o}
=\frac{R_Sr_Z}{R_S+r_Z}
=R_S\parallel r_Z\approx r_Z
$$

Lower is better.

### Line regulation

$$
S_V=\frac{\Delta V_L}{\Delta V}
=\frac{r_Z}{R_S+r_Z}
$$

Higher $R_S$ is better for line regulation.

### Constraint and BJT current booster

- Need higher $R_S$, but there are constraints on output current.
- Add BJT pass transistor / emitter follower:

$$
V_L=V_D-V_{BE}\approx V_D-0.7\,\text{V}
$$

- $R_S$ can be much bigger.
- Output resistance of emitter follower is roughly the resistance seen from base divided by $\beta$:

$$
S_I\approx \frac{1}{\beta}(R_S\parallel r_Z)
$$

---

## Page 10 - Three-Terminal Regulator and Switching Supply

![Page 10](assets/page_10.png)

## Three-terminal regulator

- Operational amplifier with negative feedback.

Output voltage:

$$
V_L=V_{ref}\left(1+\frac{R_2}{R_1}\right)
$$

Zener current sizing condition:

$$
R_S<R_{S,max}=\frac{V_{S,min}-V_{Z0}-r_ZI_{Z,min}}{I_{Z,min}}
$$

Notes:

- Load regulation is better because op-amp has very low output resistance.
- $V_S$ must be higher than $V_L$ by a margin:
  - drop from supply to op-amp output.
  - additional $V_{BE}$ of the output pass transistor.
- Typical dropout voltage:

$$
V_{DO,min}\approx 2-3\,\text{V}
$$

Conclusion:

- Linear regulator gives very good output.
- Efficiency is low.

## Switching power supply

Concept:

- A switching power supply is like a “transformer” for DC.
- It is controlled with a high switching frequency $f_{sw}$.
- The circuit uses complementary switching and a filter.
- Negative feedback can be used for regulation.

Generic block shown:

```text
Vin -> complementary switches -> filter -> load -> Vout
```

---

## Page 11 - Buck Step-Down DC-DC Converter

![Page 11](assets/page_11.png)

# Buck / step-down DC-DC converter

Duty cycle:

$$
\delta=\frac{T_{on}}{T_{sw}}
$$

- $f_{sw}$ is constant.
- $\delta$ may change.

Steady-state averages:

$$
V_{L,avg}=0
$$

$$
I_{C,avg}=0
$$

### During $T_{on}$

- Current flows in inductor $L$.
- Current in MOSFET $M_1$:

$$
I_Q=I_L
$$

- Current in diode $D_1$:

$$
I_D=0
$$

Inductor current increase:

$$
\Delta I_{L,on}=\frac{V_LT_{on}}{L}=\frac{1}{L}(V_{in}-V_{out})T_{on}
$$

### During $T_{off}$

- Switch is OFF.
- Diode conducts.
- Assuming diode drop neglected:

$$
V_A=0
$$

$$
V_L=0-V_{out}=-V_{out}
$$

Inductor current decrease:

$$
\Delta I_{L,off}=\frac{V_LT_{off}}{L}=-\frac{1}{L}V_{out}T_{off}
$$

Currents:

$$
I_Q=0
$$

$$
I_D=I_L
$$

Volt-second balance:

$$
\Delta I_{L,on}+\Delta I_{L,off}=0
$$

Therefore:

$$
V_{out}=\delta V_{in}
$$

Buck output is always lower than input.

> **[本项目]**: **PWM 电机调速本质上就是一个 Buck 变换器**。
> - H 桥高侧 MOSFET = Buck 的开关 $M_1$；内置续流二极管 = Buck 的 $D_1$
> - 电机绕组电感（数 mH）= Buck 的储能电感 $L$
> - 占空比：$\delta = \text{duty} / 4095$（12-bit，0~4095）
> - 电机两端平均电压：$V_{motor} = \delta \times V_{VM}$（VM 是电机电源，5~15V）
> - FPGA 以 50MHz/4096 ≈ **12.2kHz** 开关，对应 Buck 的 $f_{sw}$
> - 电机绕组 $L$ 足够大、$f_{sw}$ 足够高，运行在 **连续导通模式（CCM）**，电流平滑不断续，这就是为什么 12.2kHz 比 1kHz 更好（CCM 条件 $L > V_{out}/(2I_{min}f_{sw})$ 更容易满足）

### Current relations

$$
I_{in}=I_Q
$$

Since:

$$
P_{in}=P_{out}
$$

then:

$$
I_{in,avg}=\delta I_{out}
$$

Output current:

$$
I_{out}=\frac{I_{L,max}+I_{L,min}}{2}
$$

Current rating notes:

- Inductor current rating: compute $I_{L,max}$ or $I_{L,rms}$.
- Capacitor current rating:

$$
I_{C,max}=\frac{\Delta I_L}{2}
$$

and triangular-wave RMS approximation:

$$
I_{C,rms}=\frac{1}{\sqrt{3}}I_{out}
$$

### Line regulation

$$
S_V=\frac{\Delta V_{out}}{\Delta V_{in}}=\delta
$$

- It is a DC-DC converter, not a regulator by itself.
- Needs negative feedback circuit that controls $\delta$.

### Load regulation

- Output voltage does not depend on $I_{out}$ in ideal steady state.

$$
S_I=0
$$

- This ignores parasitic series resistance.
- During transients, $V_{out}\ne \delta V_{in}$.
- Valid only in continuous conduction mode (CCM).

---

## Page 12 - Buck DCM and Boost Step-Up Converter

![Page 12](assets/page_12.png)

## Buck converter: discontinuous current mode

Conditions:

- Inductor current reaches zero:

$$
I_{L,min}=0
$$

- Current change during $T_{on}$ remains the same.
- Current goes to zero during $T_2$:

$$
I_{L,max}=\frac{1}{L}V_{out}T_2
$$

$$
T_2=\frac{V_{in}-V_{out}}{V_{out}}T_{on}
$$

Now $V_{out}$ depends on $I_{out}$:

$$
V_{out}=\frac{V_{in}}{\frac{2LI_{out}}{\delta^2T_{sw}V_{in}}+1}
$$

It depends on:

$$
L,\quad \frac{T_{sw}}{I_{out}}
$$

Therefore:

$$
S_I>0
$$

Boundary current:

$$
I_{DCM}=\frac{\Delta I_{L,on}}{2}
=\frac{(1-\delta)V_{out}}{2Lf_{sw}}
$$

Without regulator:

- Output $V_{out}$ increases for low $I_{out}$.
- For $I_{out}\to0$:

$$
V_{out}=V_{in}
$$

regardless of $\delta$.

Given $I_{o,min}$, minimum $L$ for CCM:

$$
L>\frac{(1-\delta)V_{out}}{2I_{o,min}f_{sw}}
$$

Worst case $\delta=0$:

$$
L>\frac{V_{out}}{2I_{o,min}f_{sw}}
$$

Ripple condition:

$$
\frac{I_{out}}{4Cf_{sw}}<\Delta V_{out}
$$

so:

$$
C>\frac{I_{out}}{4\Delta V_{out}f_{sw}}
$$

Ripple derivation:

$$
\Delta V_{out}=\frac{\Delta I_L}{8Cf_{sw}}
$$

Worst case:

$$
\Delta I_L=2I_{out}
$$

### Current / voltage rating table shown

| Component | Current rating | Voltage rating |
|---|---:|---:|
| MOS | $\frac{2}{\sqrt{3}}I_{out}$ | $V_{in}$ |
| Diode | $\frac{2}{\sqrt{3}}I_{out}$ | $V_{in}$ |
| Inductor | $\frac{2}{\sqrt{3}}I_{out}$ | $V_{in}$ |
| Capacitor | $\frac{1}{\sqrt{3}}I_{out}$ | $V_{out}$ |

## Boost / step-up DC-DC converter

Circuit shown with inductor, MOSFET, diode, capacitor, and load.

### During $T_{on}$

- $M_1$ conducting.
- $V_A=0$.
- $D_1$ reverse-biased.
- $V_L=V_{in}$.
- Inductor current increases linearly:

$$
\Delta I_{L,on}=\frac{V_LT_{on}}{L}=\frac{1}{L}V_{in}T_{on}
$$

Currents:

$$
I_Q=I_L
$$

$$
I_D=0
$$

### During $T_{off}$

- $M_1$ cutoff.
- $D_1$ forced conduction.
- $V_A=V_{out}$.
- $V_L=V_{in}-V_{out}$.

$$
\Delta I_{L,off}=\frac{V_LT_{off}}{L}=\frac{1}{L}(V_{in}-V_{out})T_{off}
$$

Currents:

$$
I_Q=0
$$

$$
I_D=I_L
$$

With:

$$
V_{in}>0,\quad V_{out}>0,\quad V_{out}>V_{in}
$$

Volt-second balance:

$$
\Delta I_{L,on}+\Delta I_{L,off}=0
$$

Therefore:

$$
\frac{V_{out}}{V_{in}}=\frac{1}{1-\delta}
$$

---

## Page 13 - Boost Continued and Buck-Boost Start

![Page 13](assets/page_13.png)

## Boost converter continued

Current in continuous conduction mode:

$$
I_{L,max}=\frac{1}{1-\delta}I_{out}+\frac{\Delta I_L}{2}
$$

$$
I_{L,min}=\frac{1}{1-\delta}I_{out}-\frac{\Delta I_L}{2}
$$

Current from voltage source:

$$
I_{in}=I_L
$$

Power balance:

$$
P_{out}=P_{in}
$$

$$
I_{in,avg}=I_{L,avg}=\frac{V_{out}}{V_{in}}I_{out}=\frac{1}{1-\delta}I_{out}
$$

### Current ratings

Inductor:

$$
I_{L,max}\quad\text{or}\quad I_{L,rms}
$$

RMS approximation written:

$$
I_{L,rms}=\frac{2}{\sqrt{3}}I_{in,avg}
=\frac{2}{\sqrt{3}}\frac{1}{1-\delta}I_{out}
$$

Capacitor:

- When $\delta$ is low, worst case:

$$
I_{C,rms}=I_{out}
$$

Output ripple:

$$
\Delta V_{out}=\frac{I_{out}}{Cf_{sw}}=\frac{I_{out}T_{sw}}{C}
$$

### Line regulation

$$
S_V=\frac{\Delta V_{out}}{\Delta V_{in}}=\frac{1}{1-\delta}
$$

- Even worse than buck.
- Needs regulator.

### Load regulation

- Output voltage does not depend on output current $I_{out}$ in ideal CCM.

$$
S_I=0\,\Omega
$$

CCM condition:

$$
L>\frac{V_{in}}{2I_{in}f_{sw}}
$$

### Boost DCM

- Current during $T_{on}$ is the same.
- Current goes to zero during $T_2$:

$$
I_{L,max}=-\frac{1}{L}(V_{in}-V_{out})T_2
$$

$$
T_2=-\frac{V_{in}}{V_{in}-V_{out}}T_{on}
$$

Average current relation shown:

$$
\frac{1}{2}(T_{on}+T_2)I_{L,max}=\frac{V_{out}}{V_{in}}I_{out}T_{sw}
$$

DCM output voltage:

$$
V_{out}=V_{in}\left(\frac{\delta^2T_{sw}V_{in}}{2LI_{out}}+1\right)
$$

Therefore:

$$
S_I>0
$$

## Buck-boost converter

Circuit shown with switch, inductor, diode, output capacitor, and negative output.

### During $T_{on}$

- $V_A=V_{in}$.
- $D_1$ reverse-biased.

$$
\Delta I_{L,on}=\frac{V_LT_{on}}{L}=\frac{1}{L}V_{in}T_{on}
$$

Currents:

$$
I_Q=I_L
$$

$$
I_D=0
$$

### During $T_{off}$

- $D_1$ forced conduction.
- $V_A=V_{out}$.
- $V_L=V_{out}$.

$$
\Delta I_{L,off}=\frac{1}{L}V_{out}T_{off}
$$

---

## Page 14 - Buck-Boost Continued and Flyback

![Page 14](assets/page_14.png)

## Buck-boost converter continued

Assume:

$$
V_{in}>0,\quad V_{out}<0
$$

Volt-second balance:

$$
\Delta I_{L,on}+\Delta I_{L,off}=0
$$

$$
\frac{1}{L}V_{in}T_{on}+\frac{1}{L}V_{out}T_{off}=0
$$

Therefore:

$$
\frac{V_{out}}{V_{in}}=-\frac{T_{on}}{T_{off}}=-\frac{\delta}{1-\delta}
$$

$$
V_{out}=-\frac{\delta}{1-\delta}V_{in}
$$

Output is negative.

Current in inductor:

- Average current in load and in diode should be identical in absolute value because:

$$
I_{C,avg}=0
$$

Current limits:

$$
I_{L,max}=-\frac{1}{1-\delta}I_{out}+\frac{\Delta I_L}{2}
$$

$$
I_{L,min}=-\frac{1}{1-\delta}I_{out}-\frac{\Delta I_L}{2}
$$

Output current:

- Pulsed.
- If $\delta$ is high, voltage source is almost shorted to ground.

### Buck-boost DCM mode

During $T_{on}$:

$$
I_{L,on\ slope}=\frac{1}{L}V_{in}T_{on}
$$

$$
I_{L,max}=\Delta I_{L,on}=\frac{1}{L}V_{in}T_{on}
$$

During $T_2$:

$$
I_{L,max}=-\frac{1}{L}V_{out}T_2
$$

$$
T_2=-\frac{V_{in}}{V_{out}}T_{on}
$$

Average diode current equals output current:

$$
\frac{1}{2}T_2I_{L,max}=I_{out}T_{sw}
$$

Output voltage in DCM:

$$
V_{out}=-\frac{\delta^2T_{sw}V_{in}}{2LI_{out}}
$$

Thus:

$$
S_I>0
$$

## Flyback DC-DC converter

- $V_{out}>0$.

$$
V_{out}=\frac{\delta}{1-\delta}V_{in}
$$

- Transformer works at switching frequency $f_{sw}$.
- Can have multiple secondaries.
- Better to use low-side drive.

---

## Page 15 - Regulator Circuit

![Page 15](assets/page_15.png)

# Regulator circuit

Definitions:

$$
\beta=\frac{R_1}{R_1+R_2}
$$

$$
\varepsilon=\text{error}
$$

$$
\delta=HV_\varepsilon
$$

$$
H=\frac{1}{V_{kpp}}
$$

Comparator / PWM relation:

$$
T_{on}:T_{sw}=V_c:V_{kpp}
$$

Control equation:

$$
V_{out}=\delta V_{in}
$$

$$
V_{out}=\frac{V_\varepsilon}{V_{kpp}}V_{in}
$$

$$
V_{out}=\frac{G\varepsilon}{V_{kpp}}V_{in}
$$

$$
V_{out}=\frac{G}{V_{kpp}}(V_r-\beta V_{out})V_{in}
$$

$$
V_{out}=\frac{G}{V_{kpp}}\left(V_r-\frac{R_1}{R_1+R_2}V_{out}\right)V_{in}
$$

Closed-loop form:

$$
V_{out}=\frac{1}{\beta}\frac{A\beta}{1+A\beta}V_r
$$

with:

$$
A=\frac{GV_{in}}{V_{kpp}}
$$

For large loop gain:

$$
\lim_{A\beta\to\infty}V_{out}=\frac{1}{\beta}V_r
$$

$$
V_{out}=\left(1+\frac{R_2}{R_1}\right)V_r
$$

Validity note:

$$
0<V_{out}<V_{in}
$$

- For boost converter:

$$
V_{out}>V_{in}
$$

- For buck-boost converter:

$$
V_{out}<0
$$
