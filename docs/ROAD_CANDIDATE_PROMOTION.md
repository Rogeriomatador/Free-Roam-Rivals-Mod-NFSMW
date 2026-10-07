# Road Candidate Promotion

v0.0.18 introduces a hard boundary between **observing a point in the road
system** and **authorizing that point for gameplay**.

## Why

The engine exposes several useful positions through the player's IVehicleAI,
but a finite coordinate does not prove that it is tied to the exact WRoadNav
geometry we think it is, streamed, grounded, outside the camera, clear of live
vehicles or calibrated into metres.

## Observation sources

`RoadCandidateProbe` emits up to four observations:

| Source | Exact WRoadNav geometry? | Purpose |
| --- | --- | --- |
| CurrentRoad | yes | current road reference; usually not ahead |
| FutureRoad | yes | strongest current source for an ahead road point |
| SeekAhead | no | AI lookahead evidence |
| FarFuture | no | longer-range AI lookahead evidence |

SeekAhead/FarFuture deliberately inherit no gameplay authorization from a
nearby road object. They remain research/navigation evidence until an exact
road association is independently proven.

## Base inspection

Before physical units are involved, an observation must pass:

```text
position available
AND road geometry available
AND exact road association
AND WRoadNav valid
AND !deadEnd
AND forwardProjectionWorldUnits > 0
```

Passing this stage creates an exact ahead-road observation, not a spawn point.

## Metric conversion

Distance, width and segment span remain world units until:

```text
WorldMetricCalibration.verified == true
```

Only then can metre-based candidate structures be populated.

## Spawn promotion

Spawn promotion requires independent proof of:

```text
streaming
ground
vehicle-overlap check
visibility check
off-screen result
```

The resulting `SpawnCandidateInput` still passes through `SpawnSafety`, so
promotion cannot bypass population, distance or Free Roam gates.

## Staging promotion

Staging shares road/metric/streaming/ground/overlap requirements, then also
requires:

```text
junction status
obstruction status
grade
two-car geometry
```

Visibility is not a staging rejection rule. A cinematic start site may be
visible while both cars approach it naturally.

The resulting `StagingCandidate` still passes through the existing scorer.

## Readiness

The construction-readiness chain now contains:

```text
RoadLookaheadUnavailable
 -> ExactRoadCandidateUnavailable
 -> MetricCalibrationUnverified
 -> SpawnCandidateUnverified
```

This prevents generic SeekAhead/FarFuture availability from being confused
with a road point that is actually eligible for later promotion.

## Remaining live evidence

The typed pipeline is implemented, but these engine adapters still need
target-machine proof:

- metric calibration;
- streaming status at a position;
- ground validity;
- camera visibility;
- live-vehicle overlap;
- junction/obstruction/grade/two-car geometry for staging.

Missing evidence always fails closed.
