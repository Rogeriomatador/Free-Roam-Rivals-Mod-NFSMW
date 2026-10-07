# Runtime Construction Readiness

v0.0.19 extends the remaining first-spawn prerequisites into one
fail-closed readiness report.

The report exists so target-machine testing can answer:

> What is the first concrete thing still blocking the controlled construction
> experiment?

## Blocker order

The evaluator checks, in order:

1. `FrameTickProbeEnabled`
2. FrameTick hook installed
3. FrameTick calls observed
4. input-poll calls observed
5. FrameTick thread matches the input-poll thread
6. safe Free Roam observed
7. live road lookahead observed
8. at least one ahead point with exact WRoadNav geometry observed
9. complete live-vehicle spatial evidence observed
10. metric calibration verified
11. final spawn candidate verified

Only when every item passes is:

```text
readyForConstructionExperiment = true
```

## Why compare FrameTick with input polling?

Public reverse-engineering places the game action polling inside the normal
per-frame game loop. The project already observes that callback.

The opt-in FrameTick probe records its Windows thread ID without guessing the
function calling convention. Matching the FrameTick and input-poll thread IDs
gives target-machine evidence that the candidate mutation callback is running
on the same game-loop thread as the engine input pipeline.

A mismatch does not trigger a crash or fallback mutation path. It simply keeps
construction blocked.

## Runtime evidence

The health thread reads only atomics and can report a line like:

```text
Construction readiness after 8s:
  BLOCKED
  blocker=MetricCalibrationUnverified
  gameplayThreadConfirmed=1
  freeRoamObserved=1
  roadLookaheadObserved=1
  exactRoadCandidateObserved=1
  vehicleSpatialEvidenceObserved=1
  metricCalibrationVerified=0
  spawnCandidateVerified=0
```

Possible blocker names:

- `FrameTickProbeDisabled`
- `FrameTickProbeNotInstalled`
- `FrameTickNotObserved`
- `InputPollNotObserved`
- `MainLoopThreadUnconfirmed`
- `FreeRoamNotObserved`
- `RoadLookaheadUnavailable`
- `ExactRoadCandidateUnavailable`
- `VehicleSpatialEvidenceUnavailable`
- `MetricCalibrationUnverified`
- `SpawnCandidateUnverified`

## Current expected result

Public/default builds still ship with:

```ini
[Diagnostics]
FrameTickProbeEnabled=0
```

So normal play is expected to report `FrameTickProbeDisabled`.

For a deliberate diagnostics session, the probe can be enabled manually. Even
then, current development builds intentionally leave metric calibration and final spawn-candidate
promotion false, so the readiness report cannot accidentally authorize live
construction.

## Safety

The readiness evaluator is pure domain logic.

The runtime adapter only feeds it:

- atomic callback counters;
- first-observed thread IDs;
- whether safe Free Roam was observed;
- whether road lookahead was observed;
- whether at least one ahead candidate has exact WRoadNav geometry;
- whether the active PVehicle set has complete, valid spatial OBB evidence.

It does not:

- construct or kill PVehicle objects;
- mutate AI;
- write input;
- write career state;
- infer metric calibration from raw coordinates;
- promote a road point automatically.

The actual SpawnExperiment state machine remains a separate later stage.
