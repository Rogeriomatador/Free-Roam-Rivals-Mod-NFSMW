# Player Road Navigation Probe

v0.0.14 expands the read-only bridge from the validated player PVehicle to its
native IVehicleAI road-navigation state.

The purpose is deliberately narrow:

> collect engine-owned lookahead geometry that can later seed spawn and
> cinematic-staging candidates without inventing arbitrary XYZ positions.

## Data path

The probe follows only objects already independently validated by the runtime:

```text
MWSDK live IVehicle registry
  -> Human IVehicle identified
  -> NFSPluginSDK PVehicle registry independently finds player
  -> PVehicleEx::ValidatePVehicle
  -> PVehicle::GetAIVehiclePtr()
  -> IVehicleAI read-only getters
       GetCurrentRoad()
       GetFutureRoad()
       GetSeekAheadPosition()
       GetFarFuturePosition()
       GetFarFutureDirection()
```

No guessed IVehicle-to-PVehicle pointer subtraction is used.

## Captured road geometry

For CurrentRoad and FutureRoad the runtime records:

- WRoadNav validity
- segment index
- lane index
- segment time
- curvature
- left-to-right road width
- segment start/end span
- dead-end state
- road occlusion counter
- avoidable occlusion counter
- occluded-from-behind flag
- road position
- forward vector
- left/right positions
- start/end positions

It also records:

- player position
- SeekAheadPosition
- FarFuturePosition
- FarFutureDirection
- player-to-seek distance
- player-to-far-future distance
- projection of both lookahead points onto the current/future road forward vector

## World units are not metres

The raw geometry is now explicitly named `WorldUnits`.

The project does **not** assume that one NFSMW coordinate unit equals one
metre. Public SDK layouts prove the fields and functions, but they do not prove
the physical world-unit scale for the supported executable.

This matters because gameplay tuning is written in metres:

```text
spawn minimum   300-350 m
staging window   35-220 m
```

v0.0.14 therefore adds a separate `WorldMetricCalibration` contract.

A numeric scale is unusable until:

```text
calibration.verified == true
AND worldUnitsPerMeter is finite
AND worldUnitsPerMeter > 0
```

Until that happens:

- SpawnSafety rejects a candidate with `DistanceScaleUnverified`.
- StagingPlanner rejects metre-based geometry whose metric scale is not verified.
- RoadNav telemetry remains research evidence only.

This prevents a future adapter from accidentally feeding a raw coordinate
delta into a metre threshold.

## Runtime log shape

The heartbeat/state log can now include fields such as:

```text
roadNav=[
  curSeg:...
  curLane:...
  curWidthWorld:...
  curSpanWorld:...
  curSegTime:...
  curCurve:...
  curRoadOcc:...
  curAvoidOcc:...
  futureSeg:...
  futureLane:...
  futureWidthWorld:...
  navGapWorld:...
  seekDistWorld:...
  seekProjWorld:...
  farDistWorld:...
  farProjWorld:...
  playerPos:(...)
  seek:(...)
  far:(...)
  units:world
]
```

The exact values must come from the target game installation. This document
does not invent sample coordinates.

## Promotion criteria

A lookahead point is not automatically a valid spawn point.

Before any point can become a real spawn/staging candidate, target-machine
captures must establish:

1. CurrentRoad/FutureRoad remain stable during normal Free Roam driving.
2. SeekAhead/FarFuture remain on navigable road across Rosewood, Camden and Downtown.
3. Positive forward projection agrees with visible direction of travel.
4. Width/curvature/occlusion fields behave sensibly at straights, bends and junctions.
5. world-units-to-metre calibration is measured and explicitly verified.
6. visibility/off-screen evidence is available.
7. streaming/ground validity is independently confirmed.
8. no arbitrary extrapolated XYZ is required.

Only then should the adapter populate metric SpawnCandidate/StagingCandidate
structures.

## Safety

The road-navigation probe is read-only.

It does not:

- call PVehicle::Construct
- call PVehicle::Kill
- call IVehicleAI::SetDriveTarget
- reset road navigation
- alter player input
- change cash
- change the garage/save

The GameFrameTick probe remains separately opt-in and read-only.


## v0.0.18 typed promotion boundary

Road telemetry no longer flows directly into spawn/staging structures.

The adapter emits four typed observation sources:

```text
CurrentRoad
FutureRoad
SeekAhead
FarFuture
```

CurrentRoad and FutureRoad can carry `exactRoadGeometry=true` because their
position and segment/lane/width/curvature come from the same WRoadNav object.

SeekAhead and FarFuture remain useful navigation evidence, but their positions
are returned independently by IVehicleAI. Until a verified query proves the
exact WRoadNav that owns those positions, they remain blocked by
`RoadGeometryAssociationUnverified`.

Promotion is deliberately staged:

```text
RoadCandidateObservation
 -> exact road inspection
 -> WorldMetricCalibration
 -> independent runtime evidence
 -> SpawnCandidateInput / StagingCandidate
 -> existing safety/scoring rules
```

Spawn additionally requires verified off-screen visibility to prevent visible
pop-in. Staging does not require invisibility because both cars may naturally
drive toward a visible start site.

Runtime heartbeats can list each observed source with its current blocker,
segment/lane, world-unit distance and forward projection.
