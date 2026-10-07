# Target capture — 2026-10-07, v0.0.26

Source: user-supplied `FreeRoamRivals(4).log`, SHA-256
`1454c7a61ab526a5a27057364cb1bfd4a68674ac239cc551d2c3e35385c7e717`.
The original file is not republished here.

## Gameplay-loop result

- Supported v1.3 executable guard passed.
- Effective diagnostics in the final run: render=1, input=1, frameTick=1,
  motionCapture=0, camera=0, worldCollision=0.
- The MW05 main-loop signature resolved at one call site, but the current CALL
  target differed from the pinned speed.exe GameFrameTick destination.
- Because v0.0.26 rejected redirected targets, gameplayLoopInstalled remained 0;
  gameplay-loop calls/completions, frameTicks and challengeFallbackPresses all
  remained 0 through the final health snapshots.
- Render delivery stayed healthy, proving the ASI itself remained active.

The redirect is consistent with WidescreenFixesPack's documented MW05 behavior:
its `MainLoop(float)` wrapper patches this exact CALL and forwards to the
original after running its own callbacks. v0.0.27 recognizes only that named
module owner; other redirects remain blocked.

## Spatial result

The v0.0.26 validity fix materially changed target behavior. Early complete
fleets reported zero failed spatial reads. With one live box, one model was
learned; with two live boxes, two models were learned and verified. Later the
registry grew and some active entries again failed spatial reads, so complete
fleet occupancy was correctly treated as unverified.

The pending rival remained `gti`; no exact GTI footprint was learned in this
capture, so the selected-model footprint gate stayed false. This is not treated
as a reason to reroll the rival.

## Motion / construction

Internal motion-scale observation again reached stable consistency near one
world unit per engine-speed-unit-second, but this does not prove physical metres.
Metric calibration remains false.

No vehicle was constructed. The next blocker to resolve is verified gameplay
loop delivery through the already-installed Widescreen Fix chain. After that,
world-collision ownership can be tested without moving any query onto the render
thread.
