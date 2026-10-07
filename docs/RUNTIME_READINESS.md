# Runtime Construction Readiness

v0.0.21 extends the remaining first-spawn prerequisites into one
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
10. the selected pending rival vehicle has a verified learned pre-construction footprint
11. world-face ground evidence observed on an exact road candidate
12. metric calibration verified
13. final spawn candidate verified

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
  vehicleFootprintVerified=1
  verifiedFootprintVehicleKey=0x...
  groundEvidenceVerified=1
  worldOcclusionEvidenceObserved=1
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
- `VehicleFootprintUnavailable`
- `GroundEvidenceUnavailable`
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
- whether the active PVehicle set has complete, valid spatial OBB evidence;
- whether a configured catalog model has enough consistent live samples to build its footprint before construction;
- whether an exact road candidate has produced a valid world-face ground hit on the confirmed gameplay thread.

It does not:

- construct or kill PVehicle objects;
- mutate AI;
- write input;
- write career state;
- infer metric calibration from raw coordinates;
- promote a road point automatically.

The actual SpawnExperiment state machine remains a separate later stage.


## v0.0.21 world-collision execution rule

`WorldCollisionDiagnosticsEnabled=1` does not by itself permit a collision
query. The runtime also requires:

```text
FrameTickProbeEnabled=1
AND FrameTick hook installed
AND FrameTick calls observed
AND input polling observed
AND FrameTick thread == input thread
AND current callback thread == input thread
```

Only then can the input/gameplay callback consume a collision request queued by
the render sampler. This keeps WCollisionMgr traversal away from EndScene.


## v0.0.22 lifetime correction

Readiness fields now describe the latest safe sample. Ground results have a
500 ms lease and must match the current generation, both player identities,
road network, race status and profile. Footprint readiness belongs to the
selected pending rival model, is recomputed each sample and can be revoked by
a later inconsistent observation. These diagnostics never authorize mutation.

## v0.0.26 verified-loop alternative

The unobserved legacy input-poller adapter is no longer installed. A unique
WFP main-loop CALL with cdecl-float cleanup must agree with the independently
pinned GameFrameTick target before a typed hook is enabled. The evaluator has
explicit sourceVerified/threadConsistent/completedCount/threadId fields for
this alternative. FrameTick must still be enabled and installed/observed, and
its entry thread must match the loop completion thread. Native input counts
are not synthesized. This replaces the unconfirmed input ABI assumption;
entry/completion are one function's boundaries, not independent input proof.
See GAMEPLAY_LOOP_RECOVERY.md for exact provenance and target-test limitations.
All remaining geometry, footprint, ground, physical metric and final candidate
checks still apply. The runtime still never authorizes construction.
