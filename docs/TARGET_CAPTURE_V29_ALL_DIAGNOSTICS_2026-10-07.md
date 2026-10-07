# Target capture: v0.0.29-dev, all diagnostics, Golf GTI (2026-10-07)

Provenance: this audit was supplied in Claude's patch. The original log was
not supplied for this review, so the quoted measurements below have not been
independently reproduced here. They are historical reported evidence, not a
new target-game test of v0.0.30.

Session 16:31:12 to 16:33:48 (about 2.5 minutes), supported speed.exe, Free Roam,
player driving a Golf GTI, all diagnostics enabled, all Experimental flags 0.
Only one log was received (two uploads shared the same file name).

## Closed by this capture

| Gate | Evidence |
| --- | --- |
| Metric calibration | `World metric calibration verified: source=MW05_GetAbsoluteSpeed_mps worldUnitsPerMeter=1.00159 window=20 cv=0.01128` at 16:31:34; `metricVerified=1` in 18 of 20 motion observations. absolute/speed ratio 1.00000 throughout. |
| Ground | `ground=valid` in 10 of 10 world-collision readings; `groundDeltaWorld` 0.001 to 0.722, `grade` 0.008 to 0.149 (v0.0.28 showed 6.288 / 20.567). Axis canonicalisation worked. |
| Fleet occupancy | `failedSpatialReads=0` in 19 of 19 samples. |
| Selected rival footprint | `vehicleFootprintVerified=1`, key `0x79054B53` (Rico/GTI) once the player drove the GTI. Not yet obtainable from game data. |

## Still blocking (final blocker: SpawnCandidateUnverified)

- Only the CurrentRoad candidate was exact; it is the player's own road point
  (0.2 to 2.0 world units away, often `occupied`). FutureRoad is `DeadEnd`;
  SeekAhead and FarFuture stay `RoadGeometryAssociationUnverified`.
- `streamingVerified=0` in 21 of 21 readings.
- `spawnVisibilityVerified=0` in 21 of 21; primary camera `coherent=1` in only 3 of 21.
- Readiness progression: 8 s FreeRoamNotObserved, 38 s and 68 s and 128 s
  SpawnCandidateUnverified, 98 s ExactRoadCandidateUnavailable.

## Comparison with the v0.0.28 capture

v0.0.28 (other car, 2 to 13 live vehicles): `failedSpatialReads=0` 15 of 15,
`catalogFootprintKey=0x0` throughout (GTI never driven), blockers
VehicleFootprintUnavailable / ExactRoadCandidateUnavailable, ground mostly
`unverified`, metric stable but `metricVerified=0` (promotion code arrived in v0.0.29).
