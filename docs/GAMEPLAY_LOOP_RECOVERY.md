# Gameplay loop and fallback input — v0.0.26

The v0.0.25 target capture proves render delivery but inputPolls=0. The pinned
SDK's on_poll adapter hooks 0x6349B0 as void cdecl(), publishes its original after
activation and offers no independent ABI/delivery proof. The supplied log does
not identify the exact failure, and contradictory RE input-table labels are not
used as new offsets. v0.0.26 removes that unobserved adapter from runtime use.

## Verified alternative

[WidescreenFixesPack MW05](https://github.com/ThirteenAG/WidescreenFixesPack/blob/2b34384ddc038c99b730a993d6a793adfd855eaf/source/NFSMostWanted.WidescreenFix/dllmain.cpp)
wraps MainLoop as `void __cdecl(float TickerDifference)` and discovers its CALL
through `E8 ? ? ? ? A0 ? ? ? ? 83 C4 ? 84 C0 74 ? C6 05`.
The bridge additionally requires stack cleanup=4, a data operand inside the
supported executable, a unique CALL site, and a rel32 destination matching the
[pinned SDK GameFrameTick](https://github.com/s-b-repo/nfsmw-2005-sdk/blob/3b3d05b9194844883aa42d75dd7aae66838092ff/include/nfsmw_sdk/functions.h)
RVA 0x263D30. PE section reads are bounded and guarded. A missing, ambiguous or
redirected CALL fails closed; there is no ABI guess, replacement CALL patch or
silent bypass of another mod's wrapper.

MinHook hooks that verified function entry, publishes its trampoline before
enabling it and forwards the unchanged float to the original. Pre/post
observations run only at the outermost depth. A different observed thread
permanently revokes callback delivery while originals continue to chain.
The callback counters and thread are reported separately as gameplayLoopCalls,
gameplayLoopCompleted, gameplayCallbacks and gameplayLoopThread. inputPolls
remains zero; main-loop observation does not claim native action-mirror polling.

The fallback Windows virtual key is read after the original game update, only
while the game process is foreground. Each rising edge is queued once and
logged. Native horn mapping and encounter dispatch are still unimplemented.

## Gameplay ownership and unchanged construction blockers

FrameTickProbeEnabled remains opt-in. Its counter now observes the verified
function's entry rather than a second guessed-ABI mid-hook. Readiness accepts
an explicitly identified, completed, consistent loop callback as an alternative
to the older input/frame thread comparison. This is pre/post evidence from one
known function, not independent confirmation from an input poller. Missing
source/delivery/owner evidence keeps that path blocked.

Opt-in world-collision queries may run only in this post-update callback, with
FrameTick enabled, verified source, completed original and current-thread match.
The existing current-world identity and 500 ms request/result lease checks
remain in effect. They are never moved to EndScene. Physical metric, streaming,
complete visibility, selected-model footprint and final candidate gates remain
required; spawning, AI and economy/garage writes remain disabled.

## Next target test

Close the game, replace only scripts/FreeRoamRivals.asi and retain the edited INI.
Under [Diagnostics], keep InputProbeEnabled=1, enable FrameTickProbeEnabled=1,
and set MotionCaptureEnabled=0; another constant-speed run is unnecessary.
Leave WorldCollisionDiagnosticsEnabled=0 for this first callback check.
Enter Free Roam for 30–45 seconds and press/release the configured fallback key
(default G) three times while the game is focused. It will only log the edges;
no race starts. Send the full log, including discovery and periodic health.
A redirected/missing CALL needs review; do not disable the executable guard.

Native tests validate cached pointers, float bit/argument forwarding, original
completion, recursion and permanent thread-change revocation using test code.
They do not prove target-machine gameplay delivery. Spatial construction tests
exercise the factory used by the adapter and invalid/incomplete-fleet rejection.
