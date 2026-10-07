# Runtime Rival Spawn Contract

This document defines the exact safety contract that must be satisfied before
Free Roam Rivals is allowed to create a live Racer in Rockport.

## v0.0.8 implementation status

The contract is now represented by executable domain code rather than
documentation alone.

Implemented and unit-tested:

- safe Free Roam environment gate
- supported-executable gate
- loading/NIS/fade rejection
- player availability + independent PVehicle cross-check
- road-network availability
- stable-sample window
- population budget
- vehicle/candidate/ground validity
- overlap rejection
- hidden/off-screen requirement
- minimum-distance rule
- long-distance streaming-proof rule
- world-generation tracking
- generation-scoped runtime rival handles

The ASI can optionally log a **read-only spawn preflight** when
`ExperimentalSpawnEnabled=1`. A READY preflight means only that the world
environment is eligible for the next research step. It does **not** construct a
car in v0.0.8.

Still blocked before first mutation:

1. obtain a live road-relative candidate with enough streaming/visibility proof;
2. establish a dedicated gameplay-thread mutation callback;
3. prove Construct -> registry -> AI -> road-nav -> cleanup as one lifecycle.

The D3D9 EndScene callback remains observation-only.

## Engine chain

Public MW05 research currently gives us this chain:

This is a research hypothesis, not a verified callable ABI. The supplied dump
findings were not independently reproduced here. The separately published NFRR
implementation instead uses a Traffic seed followed by Racer conversion and
AI revalidation; neither route is implemented or promoted by this document.
See [streaming, navigation and cache findings](NATIVE_ROAMING_STREAMING_RESEARCH.md)
before treating a query name, global loading flag or inactive count as proof.

```text
Vehicle selection
  -> pvehicle key exists
  -> FECustomizationRecord prepared
  -> VehicleParams
       DriverClass::Racer
       vehicle key
       direction
       position
       customization
       SnapToGround | CalcPerformance
       GRaceStatus as IVehicleCache
  -> PVehicle::Construct
       game constructor @ 0x689820
  -> PVehicle
       mAI -> IVehicleAI
  -> Racer host / goal
       CreateAIVehicleRacerInstance @ 0x43EF70
       CreateAIGoalRacerInstance    @ 0x43D330 (0x43D388 is interior; see EXE_DUMP_V1_FINDINGS.md)
       SetAIRacerGoal / PushAIGoalByHash
  -> WRoadNav
  -> native AI tick
```

The important rule is that creating a PVehicle alone is not enough. A complete
rival needs a valid AI host, goal and road navigation state.

## Pre-spawn gates

Every gate must pass:

```text
supported executable
AND FreeRoamCandidate
AND GRaceStatus exists
AND GRaceStatus::PlayMode == Roaming
AND !GRaceStatus::mIsLoading
AND !IsInNIS
AND !fade
AND valid Human IVehicle
AND independent player PVehicle cross-check
AND road network exists
AND stable Free Roam sample window satisfied
AND selected pvehicle key exists
AND spawn point passes road/visibility validation
AND current live rival count < population budget
```

Any failed gate means: do nothing.

## Raw world units versus metres

WRoadNav and IVehicleAI lookahead vectors are engine/world coordinates. The current public SDK evidence proves field/function identity but does not by itself prove a physical metre scale for the supported executable.

v0.0.14 therefore treats these values as `world units` and adds an explicit calibration gate.

A raw coordinate delta may not populate `distanceFromPlayerMeters` unless:

```text
WorldMetricCalibration.verified == true
AND worldUnitsPerMeter is finite
AND worldUnitsPerMeter > 0
```

SpawnSafety returns `DistanceScaleUnverified` otherwise. Staging candidates likewise require `metricGeometryVerified`.

This prevents a seemingly plausible raw distance from silently crossing the 300-850 m spawn policy or the 35-220 m staging policy.

## Spawn point contract

Initial spawn candidates must be road-relative, not arbitrary XYZ points.

Candidate must provide:

- valid road segment
- lane/forward vector
- ground-valid position
- enough longitudinal space
- no immediate overlap with player
- no immediate overlap with another live vehicle
- outside no-spawn radius
- preferably occluded/off-screen

Initial distance band remains conservative:

```text
< 300 m      forbidden
300-850 m    candidate when hidden/off-screen
> 850 m      only when road + streaming state are known valid
```

The default INI is even more conservative and starts procedural creation at
350 m.

## AI bootstrap

The first experimental rival should use the engine's Racer driver path only
after its full lifecycle can be observed and reversed safely.

Do not manually inject steering/gas every frame as the default design.

Preferred order:

1. Construct Racer PVehicle.
2. Verify PVehicle registry contains it.
3. Verify live IVehicle registry contains DriverClass::Racer.
4. Resolve IVehicleAI.
5. Initialize/reset road navigation.
6. Set conservative drive speed.
7. Install/verify native Racer goal.
8. Observe several seconds of native AI movement.
9. Only then expose the object to EncounterDirector.

If Racer construction proves to require stock-race state, the controlled S1
probe may temporarily use Traffic only to validate physical construction and
cleanup. That must not be presented as the final rival implementation.

## Roaming behavior

A free-roam rival should not require a stock race route.

For the first prototype, roaming may use road-relative drive targets:

```text
current road
 -> future road
 -> point ahead
 -> SetDriveTarget
 -> ResetDriveToNav when required
```

The final system should prefer native goal/path behavior wherever possible.

## Interest / provoke integration

Once stable:

```text
Roaming
 -> player enters InterestRadius
 -> personality evaluates interest
 -> rival picks an approach target
 -> MatchSpeed / OvertakeAndSlow / PullAheadAndWait
 -> ChallengeAvailable
```

The rival must never continuously target the player's exact transform because
that tends to produce collisions and unnatural chasing.

## Cleanup contract

Cleanup is as important as creation.

The rival must be removed/invalidated safely when:

- world leaves Free Roam
- stock race begins
- NIS begins
- garage/safehouse transition begins
- player vehicle becomes invalid
- road network disappears
- rival is destroyed
- plugin detects a registry mismatch
- world unload starts

Preferred cleanup research order:

```text
IVehicleAI::UnSpawn()
 -> verify registries
 -> PVehicle::Kill() only through verified lifecycle
 -> clear all mod-owned pointers/handles
```

Never call Kill blindly while engine subsystems still own the object.

## Pointer policy

No live engine pointer survives a world transition.

The mod stores stable mod-side rival IDs. Runtime PVehicle/IVehicle/IVehicleAI
pointers are session handles only and must be reacquired/validated.

v0.0.8 enforces this in the runtime-handle model with a monotonically
increasing world generation. A handle from generation N is invalid in N+1 even
if an address happens to be reused by the game.

## Experimental enablement

The experimental spawn feature remains behind:

```ini
[Experimental]
ExperimentalSpawnEnabled=0
ExperimentalAIControlEnabled=0
StableFreeRoamSamplesBeforeSpawn=6
```

Even after mutation code exists, default public builds keep these switches off
until create/drive/cleanup tests pass.

## Promotion criteria

`rivalSpawnExperimentVerified` may become true only after:

- 50+ create/drive/cleanup cycles
- multiple Free Roam -> menu -> Free Roam transitions
- garage transition
- stock race transition
- pursuit active/inactive cases
- rival destruction case
- no stale PVehicle/IVehicle entries
- no leaked Racer entries
- no crash on game exit
- compatibility test with the current ASI stack


## v0.0.13 lifecycle controller

The S1 create/verify/cleanup experiment is now represented by executable,
engine-independent state rather than a loose sequence in runtime code.

```text
AwaitConstruction
 -> VerifyRegistries
 -> VerifyAI
 -> ObserveMotion
 -> Cleanup
 -> VerifyRemoval
 -> Succeeded
```

Every owned-vehicle failure diverts through cleanup before reaching `Failed`.
A failure before ownership may fail immediately.

The controller emits explicit one-shot requests:

- `requestConstruct`
- `requestCleanup`

It never calls PVehicle/AI APIs itself. The future engine adapter must execute
those requests only from a callback that has passed the target-machine
GameFrameTick validation and only with a candidate that passed SpawnSafety.

Success is intentionally strict: even after native AI movement is observed,
the experiment still removes the car and requires consecutive samples showing
it absent from both the PVehicle and live IVehicle registries. A cleanup
timeout disables further spawn experimentation for the session.
