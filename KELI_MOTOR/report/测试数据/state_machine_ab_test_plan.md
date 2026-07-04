# State Machine A/B Startup Test Plan

## Purpose

The purpose of this test is to check whether separating `STARTING` and `RUNNING` improves the low-speed startup response.

The report should not claim that the state machine reduces overshoot unless this test provides supporting data. Without this comparison, the report should only say that the state machine provides a clear place for startup-specific limits and handoff handling.

## Test Versions

### Version A: No Startup State Machine

Use a simple enable/deadband logic:

- If potentiometer ADC is below the stop threshold, motor command is disabled.
- If potentiometer ADC is above the start threshold, normal cascade PI control is enabled.
- Do not use a separate `STARTING` state.
- Do not use startup-only current limit, startup-only integral clamp, or START-to-RUN handoff reduction.
- Keep the normal PI output limits and general anti-windup.

This version is used as the baseline.

### Version B: Current State Machine

Use the current implementation:

- `IDLE`
- `STARTING`
- `RUNNING`
- `STOPPING`
- startup current limit
- startup speed-loop integral clamp
- handoff reduction before entering `RUNNING`
- target ramp and duty slew-rate limit

This version is used as the improved implementation.

## Conditions to Keep the Same

Use the same hardware and test conditions for both versions:

- Same motor and load.
- Same power supply voltage.
- Same PI gains.
- Same deadband thresholds.
- Same target-speed command.
- Same UART telemetry format.
- Same initial condition: motor fully stopped before each test.

If possible, repeat each startup test at least five times.

## Suggested Test Points

Test low-speed startup first, because this is where static friction has the strongest effect.

| Test | Target Speed | Reason |
| --- | --- | --- |
| 1 | 15 RPM | Near minimum run command |
| 2 | 20 RPM | Low-speed startup region |
| 3 | 30 RPM | Moderate startup command |

## Data to Record

Record the following telemetry during each startup:

- state
- target speed
- ramped target speed
- measured RPM
- target current
- measured current
- duty cycle
- potentiometer ADC value

The log should start before the motor is enabled and continue until the speed is stable.

## Metrics

For each test, calculate:

- peak RPM after startup
- overshoot percentage
- time from enable to first motion
- time from enable to reaching target speed
- settling time
- maximum target current
- maximum duty cycle

Overshoot can be estimated as:

```text
overshoot (%) = (peak RPM - final target RPM) / final target RPM * 100
```

## Expected Interpretation

If Version B has a lower peak RPM, lower overshoot, or smoother target-current and duty-cycle curves than Version A, the report can say that the state machine and startup handling reduce startup overshoot.

If the difference is small, the report should use a weaker claim: the state machine mainly improves code structure and makes startup-specific limits easier to apply.

If Version A performs better, the startup state machine parameters should be retuned before making any report claim.

## Report Wording After Test

If the comparison supports the state machine:

```text
The A/B startup test shows that the startup state machine reduces the peak startup speed and limits the stored control value before normal running. Therefore, the state machine is used to apply startup-specific current limiting, integral clamping and handoff handling.
```

If the comparison is not available:

```text
The startup state machine is introduced to separate startup handling from normal running. It provides a clear STARTING phase where startup-only current limiting, integral clamping and handoff handling can be applied.
```
