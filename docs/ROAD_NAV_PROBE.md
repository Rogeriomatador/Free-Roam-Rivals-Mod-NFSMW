# Player Road Navigation Probe

v0.0.9-dev adds a read-only diagnostic bridge from the validated player
PVehicle to its IVehicleAI road-navigation state.

The probe exists to answer one concrete question before the mod mutates the
world:

> Which engine-owned road-relative points are stable enough to become spawn,
> approach and cinematic-staging candidates?

## Data path

The probe intentionally follows objects already independently validated by the
runtime bridge:

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

## Captured geometry

For CurrentRoad and FutureRoad the log records:

- WRoadNav validity
- segment index
- lane index
- segment time
- curvature
- left-to-right road width
- dead-end state
- road and avoidable occlusion counters
- occluded-from-behind flag
- road position
- forward vector
- start position
- end position

It also records:

- player position
- SeekAheadPosition
- FarFuturePosition
- FarFutureDirection
- straight-line player-to-point distance
- projection of the point onto the current/future road forward vector

## Units

The diagnostics deliberately label geometry as `world` units rather than
claiming that every coordinate delta is already a calibrated metre.

The gameplay tuning remains expressed in metres conceptually, but the mapping
must be checked against real Rockport captures before a raw engine distance is
used as a spawn threshold.

## Log example shape

```text
Player road-nav generation=3 sample=80 ai=0x...
 player=(...)
 current=[valid=1,seg=...,lane=...,width=...,curve=...,pos=(...),fwd=(...),...]
 future=[...]
 seekAhead=(...) seekDist=... seekProj=...
 farFuture=(...) farDist=... farProj=... farDir=(...) units=world
```

The exact values must come from the user's game; this document does not invent
sample coordinates.

## Configuration

```ini
[Diagnostics]
RoadNavProbeEnabled=1
RoadNavLogEverySamples=20
```

With the default runtime sampling cadence, the default logging interval is
roughly ten seconds at 60 FPS. It is intentionally rate-limited so a normal
drive can produce useful coverage without flooding the log.

## Promotion criteria

Road-nav data is not automatically a valid spawn point.

Before a point may be promoted into the spawn planner, captures must establish:

1. the field remains valid while driving normally;
2. it stays on navigable road across Rosewood, Camden and Rockport;
3. forward projection agrees with the visible direction of travel;
4. width/curvature values behave sensibly at straights, bends and junctions;
5. the candidate can be paired with visibility/occlusion evidence;
6. the location is loaded/streamed when used;
7. no arbitrary extrapolated XYZ is required.

Only after those checks should the runtime construct a vehicle.

## Gameplay-thread evidence

The runtime health line also records the first thread ID observed by the D3D9
render callback and by the engine input-poll callback:

```text
renderThread=... inputThread=... sameThread=0|1
```

This does not by itself authorize gameplay mutation. It gives us an in-game
fact about callback execution so the construction probe can avoid being placed
on an arbitrary background thread.

## Safety


The probe is read-only and is sampled only while the runtime classifies the
game as a FreeRoamCandidate.

v0.0.9 does not:

- call PVehicle::Construct
- call PVehicle::Kill
- call IVehicleAI::SetDriveTarget
- change player AI control
- modify cash
- modify the garage/save

The D3D9 EndScene hook remains an observation point, not the future mutation
thread.
