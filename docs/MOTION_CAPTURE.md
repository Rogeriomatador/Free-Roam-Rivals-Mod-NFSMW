# Target-machine motion capture — v0.0.24

This is a read-only measurement procedure. It does not authorize construction,
change game physics or set `WorldMetricCalibration.verified`.

## Capture

1. Install the current development ZIP on the supported executable:
   MD5 `C0516B485065FABDD69579816B5DF763`, size 6029312 bytes.
2. In `scripts/FreeRoamRivals/FreeRoamRivals.ini`, under `[Diagnostics]`, set
   `MotionCaptureEnabled=1`. Keep `RenderProbeEnabled=1` and
   `RuntimeSampleEveryFrames=30` initially. No FrameTick or world-collision
   setting is required for this capture.
3. Enter normal Free Roam. Drive straight at steady positive speed, on the
   ground, for 60–90 seconds. Note the car, district, HUD speed units and any
   collisions, jumps, pauses, Speedbreaker or other mods affecting simulation.
4. Repeat in multiple cars and districts. Record a separate named copy of the
   complete `scripts/FreeRoamRivals/FreeRoamRivals.log` for each test session.
   Keep the executable guard and boot lines, and retain raw samples unchanged.
5. Disable `MotionCaptureEnabled` afterwards. Per-sample logging flushes the
   existing diagnostic log and can affect frame timing or grow the file.

Compare the stock HUD with `speedometer` samples: the research suggests this
getter is a drivetrain speed in MPS, while the HUD converts it to KPH/MPH.
Do not expect `GetSpeedometer / GetSpeed` to equal 3.6 merely because the HUD
uses KPH. Wheel slip and gearing can make the two getters differ.

## Audit

The ZIP includes a standard-library Python 3 script under `tools/`:

```console
python tools/analyze_motion_capture.py FreeRoamRivals.log > motion-report.json
```

The command reads the log and writes JSON to stdout. Exit code 0 means records
were present and their pair/window arithmetic matched; 1 indicates absent,
malformed or inconsistent records. A zero exit code is **not** a calibration
approval. Direction warnings require inspection even with exit code 0.

`FRR_MOTION_V1` lines carry raw positions, positive-speed cross-checks, world
linear velocity, slip angle, steady-clock pair duration, monotonic milliseconds,
pair acceptance and reported window statistics. Floats use round-trip precision.
The boot capture ID and opaque cohort ID distinguish runs and player/profile/
world changes without logging a profile name or raw player pointers. Exact
runtime model key and world generation remain visible.

The auditor independently recomputes pair acceptance, displacement/speed/time,
the last 120 accepted pairs, CV and stability. It also checks monotonic timing,
duplicate/missing/nonfinite fields and impossible metric promotion. A malformed
row breaks continuity; the auditor never joins across it. Summaries remain
separate for each capture/cohort/generation/model.

Velocity direction versus displacement is reported as a warning below cosine
0.995 at either endpoint. This identifies turns/slides without silently filtering
the measured scale toward a desired result. It cannot detect every teleport or
transient collision between samples.

## Remaining proof

- Elapsed time is wall time, not verified simulation time. A stable ratio can
  describe a slow-motion regime. Speedbreaker must be recorded/excluded and
  simulation-time semantics established before metric promotion.
- A stable window is consistency evidence only; the observer now replaces old
  samples and immediately revokes stability on any rejected pair.
- District labels and actual HUD observations are supplied by the test operator;
  the mod does not claim to read a district or HUD digit in this release.
- Capture on the exact target executable, corroborated getter semantics and
  cross-car/district results are still absent from the repository.
- Streaming section activation, complete camera visibility and the controlled
  gameplay-thread create/cleanup cycle remain independent gates.

Send the complete logs together with the car/district/HUD/test notes for review.
The audit JSON always contains `metricVerified: false`.
