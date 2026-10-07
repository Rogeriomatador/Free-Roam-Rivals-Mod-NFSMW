# Target capture — 2026-10-07, v0.0.28 all diagnostics

Source: user-supplied `FreeRoamRivals(8).log`. The original file is not
republished here.

## Confirmed

- v0.0.28 loaded on the supported 6,029,312-byte speed.exe with MD5
  C0516B485065FABDD69579816B5DF763.
- Render, fallback input, FrameTick, motion, camera and world-collision
  diagnostics ran together.
- The Widescreen Fix main-loop chain stayed verified and thread-consistent.
- The live-IVehicle spatial change is confirmed on target. Across safe samples,
  the spatial registry count follows the live vehicle count and
  failedSpatialReads remains zero, including a 14-vehicle sample with 14 valid
  boxes.
- Exact WRoadNav candidates continue to appear.
- Primary-camera coherence is observed intermittently.
- Motion repeatedly stabilizes close to one world unit per engine-speed-unit
  second while absolute speed and linear/local velocity agree 1:1.

## Collision result and coordinate finding

v0.0.28 produced a type-1 ground result, proving the gameplay-thread mailbox
and CheckHitWorld call can return structured world-face evidence. That sample
reported groundDeltaWorld=6.288 and grade=20.567, so it is not accepted as a
trustworthy spawn-ground result.

Source comparison then showed that NFSPluginSDK's field declaration order
(y,z,x) is not the engine UMath ABI. Reconstructed MW05 PC UMath vectors are
x,y,z with Y vertical. Existing FRR probes copied NFSPluginSDK field names
directly, so domain vectors were cyclically permuted. v0.0.29 fixes the
boundary convention instead of weakening an evidence gate.

## Remaining blockers

- Re-test ground and grade after canonical X/Y-up/Z conversion.
- Selected rival Rico uses GTI key 0x79054B53; that exact footprint was not
  observed in this capture, so selected-model overlap stays unverified.
- Full player-view coverage/render-envelope and final spawn visibility are not
  yet proven.
- Streaming proof and final spawn-candidate proof remain unavailable.
- Runtime construction and every gameplay mutation remain disabled.
