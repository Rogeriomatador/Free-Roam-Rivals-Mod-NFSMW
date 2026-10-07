# Target capture — 2026-10-07, v0.0.25

Source: user-supplied `FreeRoamRivals(3).log`, SHA-256 `022d53c685c728bb66beafb67e60b492f2568a7bde5d1b0d4ee7da910b459430`.
The original user file is preserved outside this repository; no executable path,
raw profile pointer or complete runtime log is republished here.

## Confirmed delivery

- Supported executable: size 6029312, MD5 C0516B485065FABDD69579816B5DF763.
- Actual diagnostic settings: render=1, input=1, frameTick=0, motionCapture=1.
- EndScene and Present method-entry hooks both report MH_OK on system d3d9.dll.
- First runtime delivery at 13:41:10.303; at 128 seconds renderFrames=9009,
  EndSceneCalls=9009, PresentCalls=9009, samples=301, renderThread=5168.
- The legacy SDK input-poller counter remains zero; FrameTick is disabled.

## Independent audit

The packaged Python auditor exits 0: 350 records, 0 malformed rows and 0
arithmetic mismatches. One driving cohort contains 295 samples over 123.453
seconds, model 0xC46958F8, with 288 accepted pairs and 13 direction warnings.
The final 120-pair window has worldUnitsPerSpeedUnitSecond=0.9893402779;
speedometerToSpeed=0.9973926971 and local/linear/absolute cross-checks near 1.
The observed pair range is 0.7670694193–1.0925731355; a stable window is not proof
that every interval was valid or that a physical metre is calibrated.

The user attempted to maintain 100 km/h. Median engine speed=27.7605953; under
the hypothesized MPS conversion this is 99.93814308 km/h. That agreement is a
useful operator cross-check, not a readout of HUD digits. The earlier car/tuning
notes remain user reports; this series itself only proves one runtime model key.
Wall-clock duration is not independently verified simulation time. Metric
calibration remains false, and no construction or AI is authorized by this audit.

## Other findings

An exact road candidate was observed by the 128-second health snapshot. During
normal driving the spatial registry terminates successfully but every enabled
active box is rejected (e.g. 21 boxes / 21 failedSpatialReads at 98 seconds).
The v0.0.25 adapter initializes `box.valid=false`, then calls a validator that
requires `box.valid=true`; therefore even correct read geometry fails. v0.0.26
uses a tested domain factory that initializes and validates new geometry while
retaining rejection of zero/nonfinite/non-orthogonal inputs and incomplete fleets.
Target validation of the corrected spatial reads is still required.
