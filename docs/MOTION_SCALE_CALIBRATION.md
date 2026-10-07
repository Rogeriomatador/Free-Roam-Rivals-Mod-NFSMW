# Motion / Metric Calibration Research

v0.0.17 adds read-only motion telemetry and a statistical consistency observer
for the player car.

The goal is to prove the relationship between NFSMW world coordinates and a
physical distance unit before any raw road-nav delta is allowed into metre-based
spawn/staging thresholds.

## Why this is necessary

The project already knows how to read:

- player world position;
- current/future WRoadNav;
- SeekAhead/FarFuture positions.

Those are engine coordinates.

The gameplay design, however, uses physical tuning values such as:

- spawn at roughly 350-850 m;
- staging 35-220 m ahead.

Treating an arbitrary coordinate delta as metres would make those safety gates
meaningless.

## Read-only player motion sources

The validated PVehicle exposes:

- `GetSpeed()`
- `GetSpeedometer()`
- `GetAbsoluteSpeed()`
- `GetLocalVelocity()`
- inherited `GetLinearVelocity()`
- `GetPosition()`
- wheel-on-ground count
- slip angle

v0.0.17 reads these only after the existing PVehicle validation path succeeds.

No speed, velocity or transform setter is called.

## Community evidence, not promotion proof

A public MW05 mod using the same PVehicle interface contains this operation:

```cpp
pvehicle->SetSpeed(4.0f);  // 15km/h
```

Since 4 m/s is 14.4 km/h, this is strong evidence that the speed interface may
use metres per second.

It is still treated as a hypothesis, not as sufficient proof for
`WorldMetricCalibration.verified`.

The source used during research is the MW05 BlurPowerups effect in
`berkayylmao/NFS-Chat-Chaos-Mod`.

## MotionScaleObserver

The observer receives successive read-only player samples.

It computes:

```text
worldDistance =
  length(currentWorldPosition - previousWorldPosition)

speedUnitDistance =
  average(abs(GetSpeed())) * elapsedSeconds

observedRatio =
  worldDistance / speedUnitDistance
```

The result is deliberately named:

```text
worldUnitsPerSpeedUnitSecond
```

not `worldUnitsPerMeter`.

It also measures:

```text
GetSpeedometer / abs(GetSpeed)
GetAbsoluteSpeed / abs(GetSpeed)
abs(GetSpeed) / |GetLocalVelocity|
abs(GetSpeed) / |GetLinearVelocity|
```

These ratios help identify whether the exposed values use the same physical
unit or whether one of them already contains a MPH/KPH display conversion.

## Rejected samples

The observer rejects pairs when:

- either sample is invalid;
- either sample is outside safe Free Roam;
- either sample has fewer than 3 wheels grounded;
- elapsed wall time is too small or too large;
- engine speed is too low;
- speed changes too much between samples;
- world displacement is effectively zero;
- the computed ratio is non-finite/invalid.

This rejects sampled menus, transitions, airborne endpoints and large speed
changes. It cannot prove the absence of a teleport, turn or collision between
samples, and wall time alone cannot distinguish normal simulation from Speedbreaker.

## Stability

The current default requires at least 12 consecutive accepted samples and a coefficient of
variation no greater than 5%.

Since v0.0.24, statistics cover at most the latest 120 accepted pairs; rejection
clears the window and revokes stability. Runtime accumulation also resets on
model/player/profile/race-status/road-network/world identity changes.
Nonfinite channels, nonpositive velocity magnitudes and non-forward speed pairs
are rejected. Cumulative accepted/rejected counts are diagnostics within a cohort,
not the sample count used to determine current stability.

A stable estimate only means the relationship is internally consistent.

It does **not** set:

```text
WorldMetricCalibration.verified = true
```

## Runtime logs

When enough driving data exists, the runtime can report:

```text
Motion-scale observation:
  accepted=...
  rejected=...
  worldUnitsPerSpeedUnitSecond=...
  cv=...
  speedometerToSpeedRatio=...
  absoluteToSpeedRatio=...
  speedToLocalRatio=...
  speedToLinearRatio=...
  stable=...
  metricVerified=0
```

The ordinary runtime snapshot also exposes raw player motion values.

## Promotion criteria

Metric calibration may be promoted only after target-machine captures show a
consistent interpretation.

At minimum:

1. motion observer becomes stable during normal straight Free Roam driving;
2. `GetAbsoluteSpeed / abs(GetSpeed)` is physically sensible;
3. local/linear velocity magnitudes agree with the speed interface;
4. speedometer ratio matches the actual HUD speed behavior;
5. observed world displacement agrees with elapsed physical travel;
6. results remain consistent across multiple cars and multiple districts;
7. transitions, Speedbreaker, collisions and jumps do not contaminate accepted samples.

If evidence proves that one engine speed unit equals one metre/second, then:

```text
worldUnitsPerMeter
  = worldUnitsPerSpeedUnitSecond
```

Otherwise the verified physical conversion must be applied explicitly.

Until that proof exists, spawn and staging remain blocked by
`MetricCalibrationUnverified`.

## Pinned physical-unit research — v0.0.24

The public MW05 reconstruction at commit
`1f2cdd7996791c81a580b3f7b36b44d4f9f6719c` supplies stronger semantic evidence:

- [ConversionUtil.hpp](https://github.com/dbalatoni13/nfsmw/blob/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c/src/Speed/Indep/Tools/Inc/ConversionUtil.hpp)
  defines MPS-to-KPH/MPH conversions and inches-to-metres wheel dimensions.
- [FeSpeedometer.cpp](https://github.com/dbalatoni13/nfsmw/blob/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c/src/Speed/Indep/Src/Frontend/HUD/FeSpeedometer.cpp)
  applies those MPS conversions when rendering the selected HUD speed units.
- [PhysicsInfo.cpp](https://github.com/dbalatoni13/nfsmw/blob/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c/src/Speed/Indep/Src/Physics/PhysicsInfo.cpp)
  computes wheel diameter from rim/section/aspect dimensions, and its Mps-typed
  speedometer uses wheel radius, transmission ratio and angular speed.
- [EngineRacer.cpp](https://github.com/dbalatoni13/nfsmw/blob/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c/src/Speed/Indep/Src/Physics/Behaviors/EngineRacer.cpp)
  obtains the drivetrain speedometer from transmission angular velocity and
  explicitly converts `IVehicle::GetAbsoluteSpeed()` from MPS for NOS thresholds.

These sources support MPS semantics as an inference. They also explain why a
drivetrain speedometer may differ from chassis motion. They do not establish the
exact PC getter implementation, position integration/time basis, or the target
machine's observed scale. Reconstruction platform addresses are not imported.
No metric gate is promoted from this research alone.

Use [MOTION_CAPTURE.md](MOTION_CAPTURE.md) and the packaged offline auditor to
collect and inspect the missing measurements reproducibly.
