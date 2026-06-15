# GitHub 电机启动超调相关案例调研

## 1. 调研背景

当前电机采用速度外环和电流内环的串级 PI 控制。串口数据
`report/测试数据/196and45.md` 显示，电机从静止启动时存在明显的 RPM 冲刺：

| 工况 | 目标转速 | 峰值转速 | 估算超调 |
|------|----------|----------|----------|
| 低速固定目标 | 约 20 RPM | 30.1 RPM | 约 47% |
| 低速固定目标 | 约 20 RPM | 25.1 RPM | 约 22% |
| 中速目标 | 约 46 RPM | 55.6 RPM | 约 21% |
| 中速目标 | 约 47 RPM | 57.7 RPM | 约 23% |

这些峰值通常出现在静摩擦被击穿、duty 快速升高之后，并持续多个采样点。
因此主要问题更接近真实的机械超调，而不是单个编码器噪声点。

本次调研于 2026-06-15 进行，重点检索以下问题：

- 电机静摩擦导致的启动积分累积；
- 速度 PI 积分饱和与释放后的超调；
- 速度环和电流环串级控制的调试方法；
- START 到 RUN 的状态切换与无扰交接；
- 编码器或 Hall 反馈毛刺造成的虚假速度峰值。

---

## 2. ODrive：独立限制速度积分贡献

### 2.1 相关项目

- 项目：[odriverobotics/ODrive](https://github.com/odriverobotics/ODrive)
- PR：[Added integrator_limit to allow prevention of integrator windup #626](https://github.com/odriverobotics/ODrive/pull/626)

### 2.2 相似现象

该 PR 描述的现象与本项目最接近：

- 电机受到外力、机械限位或过大速度指令时，速度误差长期存在；
- 速度控制器的积分持续累积；
- 阻力或约束解除后，积累的积分继续输出较大转矩；
- 电机随后出现明显的欠调或超调。

ODrive 原有控制器已经具备总输出限幅和积分衰减，但贡献者指出：
总输出限幅不能保证积分项本身维持在一个较低且可预测的范围内。

### 2.3 合并的解决方法

ODrive 为速度控制器增加了独立的积分输出上限：

```cpp
vel_integrator_torque_ = std::clamp(
    vel_integrator_torque_,
    -config_.vel_integrator_limit,
    config_.vel_integrator_limit
);
```

该限制直接使用转矩单位，与比例增益和积分增益解耦。讨论中给出的示例是：
可先把积分贡献限制为最大允许电流或转矩的约 20%，再根据负载能力调整。

### 2.4 对本项目的启示

当前代码使用：

```c
#define SPEED_START_INTEGRAL_MAX 11000.0f
```

它限制的是 PI 内部积分状态，实际对应的电流贡献为：

```text
I_term_mA = speed_pid.ki * speed_pid.integral
```

因此在线修改 `ki` 后，同一个 `SPEED_START_INTEGRAL_MAX` 会产生不同的电流上限。
更稳定的设计是直接限制积分项产生的电流：

```c
#define SPEED_START_I_TERM_MAX_MA 30.0f
```

实现时可将积分状态限制换算为：

```c
integral_limit = SPEED_START_I_TERM_MAX_MA / speed_pid.ki;
```

或者在 PID 实现中直接保存和限制已经乘过 `ki` 的积分输出。后者单位更清楚，
也更接近 ODrive 的实现。

---

## 3. SimpleFOC：先稳定电流内环，再接入速度外环

### 3.1 相关项目

- 项目：[simplefoc/Arduino-FOC](https://github.com/simplefoc/Arduino-FOC)
- Issue：[Motor very unstable in foc_current mode #146](https://github.com/simplefoc/Arduino-FOC/issues/146)

### 3.2 项目维护者的调试建议

该 issue 讨论的是电流模式不稳定和速度控制过冲。维护者建议：

1. 先在纯转矩或电流模式下调试电流内环；
2. 确认电流测量方向、零点和相位关系正确；
3. 从较低的电流 PI 增益开始；
4. 电流内环稳定后，再接入速度外环；
5. 避免过强的低通滤波，因为过大的反馈延迟也可能加剧振荡或超调。

### 3.3 对本项目的启示

本项目数据中可观察到：

- START 到 RUN 交接后，`iset` 已经明显下降；
- 但 duty 仍可能继续升高；
- RPM 随后才达到峰值。

这说明启动能量不只存储在速度外环积分中，还可能存在于：

- 电流 PI 的积分状态；
- 已经升高的 `duty_applied`；
- 电机及负载的机械惯性；
- 电流采样与滤波造成的反馈延迟。

因此仅修改速度环参数或 START 到 RUN 阈值，可能只能缓解问题。应先单独验证：

- 固定 `iset` 阶跃时，实际电流能否快速且无明显超调地跟踪；
- duty 是否在 `iset` 降低后仍长时间维持高位；
- 电流滤波是否引入了过大的相位延迟。

---

## 4. VESC：排除测速反馈毛刺

### 4.1 相关项目

- 项目：[vedderb/bldc](https://github.com/vedderb/bldc)
- Issue：[Hall sensors need more filtering, and display hall errors #182](https://github.com/vedderb/bldc/issues/182)

### 4.2 相似问题

该 issue 中，Hall 信号受到功率线反电动势和电磁干扰后产生非法跳变，表现为：

- 瞬时速度异常；
- 电机异响或扭矩突降；
- 负载越大，问题越明显。

讨论中的改进包括：

- 增加软件滤波样本数；
- 检测并记录非法 Hall 状态；
- 对不合理跳变进行拒绝或多数判决；
- 必要时增加硬件 RC 滤波；
- 评估 RC 滤波带来的相位延迟。

### 4.3 对本项目的适用性

本项目的 RPM 使用 8 点移动平均滤波。当前绿色峰值与 duty 上升同步，
而且连续多个样本高于 `setR`，所以主要现象不像单个编码器毛刺。

不过仍建议增加诊断字段：

- 每个速度周期的原始 `encoder_delta`；
- 未滤波 `raw_rpm`；
- 编码器异常跳变计数；
- START 到 RUN 的实际交接时刻。

如果 `raw_rpm` 出现单点不可能值，而 duty、电流和机械状态没有对应变化，
才应优先处理测速毛刺。不能只靠加重滤波解决，因为更大的滤波延迟可能使
START 到 RUN 的交接进一步滞后。

---

## 5. 综合判断

GitHub 案例与本项目数据共同指向以下原因链：

```text
静摩擦使电机长时间停转
        ↓
速度误差持续存在
        ↓
速度积分、电流积分和 duty 逐渐累积
        ↓
电机击穿静摩擦并开始转动
        ↓
滤波 RPM 延迟，START 状态不能立即结束
        ↓
累积能量继续加速电机
        ↓
产生 RPM 启动超调
```

最新提交将 START 到 RUN 的阈值从 15 RPM 提前到 12 RPM，方向合理，
但它只缩短了一部分交接延迟。由于速度使用 8 点移动平均，该阈值仍然建立在
延迟反馈上，因此预期可以降低峰值，但不一定能彻底消除冲刺。

---

## 6. 推荐实施顺序

### 6.1 第一阶段：完善遥测和对照测试

先不要同时修改多个控制策略。建议新增以下遥测字段：

- `raw_rpm`；
- `encoder_delta`；
- `speed_p_term_mA`；
- `speed_i_term_mA`；
- `current_p_term_duty`；
- `current_i_term_duty`；
- `duty_target`；
- `duty_applied`；
- START 到 RUN 交接事件。

固定电位器位置后，每个工况重复启动至少 5 次，记录：

- 启动成功率；
- 静止持续时间；
- 挣脱时 `iset` 和 duty；
- 交接时 RPM、`iset` 和 duty；
- 峰值 RPM；
- 最低回落 RPM；
- 稳定时间。

### 6.2 第二阶段：积分贡献按物理单位限幅

参考 ODrive，将速度积分项限制为明确的 mA，而不是内部积分状态。
初始值可先取最大电流给定 `400mA` 的约 10% 至 20%，即 `40~80mA`。

由于当前实测维持约 20 RPM 需要约 `30mA`，建议先从较保守的
`40mA` 开始，避免积分上限过低导致低速带载能力不足。

### 6.3 第三阶段：独立 START 电流斜坡

START 阶段不再依赖速度 PI 长时间积分来产生启动电流，而是使用独立且可预测的
电流斜坡：

```text
START iset: 0mA → 110mA
初始斜率: 30mA/s
达到挣脱条件后停止斜坡并进入 RUN
```

这样可以降低启动时间对目标 RPM 的依赖，并让启动能量更容易调节和复现实验。

### 6.4 第四阶段：更早且连续地检测挣脱

不建议只等待滤波 RPM 达到固定阈值。可增加独立挣脱判定，例如：

```text
raw_rpm > 3 RPM 或 encoder_delta 连续非零
并连续满足 2~3 个速度周期
```

连续判定可以兼顾抗毛刺和低延迟。阈值及连续次数必须通过实测确定。

### 6.5 第五阶段：START 到 RUN 无扰交接

当前交接采用：

```text
速度积分 × 0.4
电流积分 × 0.4
duty_applied × 0.6
```

这种方法会释放能量，但交接后的输出可能产生跳变。更完整的方法是根据交接前输出
反算 PI 积分，使两个控制器第一拍输出连续：

```text
speed_i = current_iset_before_handoff - speed_p
current_i = duty_before_handoff - current_p
```

实际实现时还要：

- 处理 `ki == 0`；
- 对反算结果执行积分限幅；
- 保证 duty 和电流单位换算一致；
- 避免同一控制周期内重复执行交接。

### 6.6 第六阶段：堵转保护

START 电流斜坡和积分限幅不能替代安全保护。建议增加：

```text
START 持续超过约 6s
且 RPM 仍接近 0
→ MOTOR_FAULT
```

进入 FAULT 后停止 PWM，旋钮回零或显式命令清除故障。

---

## 7. 不建议直接采用的做法

### 7.1 只继续降低 START 到 RUN 阈值

阈值过低可能使速度环在静摩擦尚未稳定击穿时接管，造成掉速、重新堵转或状态抖动。
降低阈值应与连续挣脱判定结合。

### 7.2 只增加 RPM 滤波

更多滤波虽然能让曲线更平滑，但会增加反馈延迟，可能使启动交接更晚。
应先记录 `raw_rpm`，确认是否真的存在毛刺。

### 7.3 同时大幅降低速度 PI 和电流 PI

这会掩盖根因，并使启动能力、带载能力和稳态误差一起变化。串级控制应先验证内环，
再逐层调试外环。

### 7.4 只观察 5Hz 串口曲线

当前串口相邻点约 200ms，无法准确描述击穿静摩擦后的快速过程。
控制器内部应保留更高频的短窗口数据，或至少记录峰值和交接事件。

---

## 8. 建议结论

当前问题与 ODrive 中的速度积分 windup 最为接近，但本项目还叠加了电流内环、
duty 斜率限制、测速滤波和状态切换，因此不能只采用单一修复。

建议最终方案为：

1. 速度积分贡献按 mA 限幅；
2. START 使用独立电流斜坡；
3. 使用连续编码器运动检测提前识别挣脱；
4. START 到 RUN 使用无扰积分反算；
5. 单独验证并整定电流内环；
6. 保留堵转超时和最大启动电流保护；
7. 使用固定目标、重复启动和高频遥测进行量化验证。

其中，最新提交的 `15 RPM → 12 RPM` 可作为低风险的初步实验，
但应视为验证早交接方向的临时步骤，而不是最终解决方案。

---

## 9. 参考链接

- [ODrive PR #626: Added integrator_limit to allow prevention of integrator windup](https://github.com/odriverobotics/ODrive/pull/626)
- [SimpleFOC Issue #146: Motor very unstable in foc_current mode](https://github.com/simplefoc/Arduino-FOC/issues/146)
- [VESC Issue #182: Hall sensors need more filtering, and display hall errors](https://github.com/vedderb/bldc/issues/182)
- [本项目启动测试数据：196and45.md](测试数据/196and45.md)
- [本项目电机控制优化实验记录](../MOTOR_TEST/电机控制优化实验记录.md)
