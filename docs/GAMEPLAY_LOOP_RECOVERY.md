# Gameplay loop and fallback input — v0.0.27

The v0.0.26 target run answered the callback question precisely. The unique MW05
main-loop call site was found, but its current destination was not the pinned
speed.exe GameFrameTick address. The supplied log reported one redirected target
and therefore v0.0.26 correctly failed closed: no gameplay-loop callbacks,
FrameTick ticks or fallback-key edges were fabricated.

## Why the redirect is expected on this installation

The exact signature used by the bridge comes from the pinned
WidescreenFixesPack MW05 implementation. That implementation replaces the same
CALL with its own `MainLoop(float TickerDifference)` wrapper, runs its callbacks,
then forwards the unchanged float to the original function. On a game where
`NFSMostWanted.WidescreenFix.asi` loaded first, seeing a redirected destination
at that call site is therefore expected.

v0.0.27 keeps the unique signature, stack-cleanup=4, in-image operand and
supported-executable checks. It now inspects the module owning the redirected
destination. Only the case-insensitive basename
`NFSMostWanted.WidescreenFix.asi` is accepted as a known chained wrapper.
Missing or ambiguous signatures, redirects to any other module, unreadable
owners and non-executable targets remain blocked.

The bridge observes the recognized wrapper entry with MinHook and calls the
existing target through the trampoline. It does not rewrite the Widescreen Fix
CALL, skip its processing, or jump directly to the original game function.
Direct unredirected installs continue to hook the independently pinned
GameFrameTick target.

## Target evidence recovered from v0.0.26

The corrected vehicle-spatial factory is now proven to work on the target for
at least complete early fleets: the log reached complete registries with zero
failed spatial reads and learned/verified model footprints. As traffic grew,
some active registry entries still lacked usable spatial reads, so complete-fleet
occupancy remains fail-closed. The selected pending rival was a GTI and its exact
footprint was not observed in the run. No construction gate is weakened here.

Motion-scale consistency again stabilized internally, but physical metric
calibration remains unverified. World collision and camera diagnostics were off.

## Next target test

1. Close the game and replace only `scripts/FreeRoamRivals.asi`; keep the edited INI.
2. Under `[Diagnostics]`, keep:
   - `InputProbeEnabled=1`
   - `FrameTickProbeEnabled=1`
   - `MotionCaptureEnabled=0`
   - `WorldCollisionDiagnosticsEnabled=0`
3. Enter Free Roam for 30–45 seconds.
4. With the game focused, press and release **G three times**.
5. Send `scripts/FreeRoamRivals/FreeRoamRivals.log`.

Expected evidence is a route line naming the Widescreen Fix owner with
`authorization=known_widescreenfix_chain`, successful MinHook installation,
nonzero gameplay-loop completions and FrameTicks, plus exactly three rising-edge
fallback presses. The key still only logs; it does not start a race.

If that passes, the next build can turn on the already guarded world-collision
mailbox and continue closing the ground/occlusion gates. Rival construction,
AI control, economy and garage writes remain disabled.
