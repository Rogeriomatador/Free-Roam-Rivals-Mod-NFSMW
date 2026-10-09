# v0.0.39 startup exit — 2026-10-09

Source: user-uploaded `FreeRoamRivals(20261009-082858).log`. The user reports
that the game starts and then closes. This file contains two appended sessions,
starting at 05:28:34.302 and 05:28:37.885 local time.

## What is actually recorded

- Both sessions identify the supported 6029312-byte executable, MD5
  `C0516B485065FABDD69579816B5DF763`, and FRR v0.0.39-dev.
- The recognized WidescreenFix gameplay wrapper hook and D3D9 observations
  install successfully. The first render and completed gameplay callbacks run.
- The sampled state is `NoWorld`, flow=1, zero player/vehicle/road pointers.
- The last recorded event in each session is the read-only post-race observation
  being revoked because the player/world are absent.
- No F8 request, construction call, pending confirmation or AI activation is
  recorded. The v0.0.39 two-frame confirmation path was not reached.
- Both module inventories list `NFSMWBartender.asi` and `X360Stuff.asi` as loaded.
  This contradicts a test setup that excludes them, but does not prove either
  caused the exit. Bartender's SetGoal code conflict was established in earlier
  captures; no live code audit or exception address is present in this capture.
- The file has no exit code, native exception address or crash dump. Its final
  line is not proof that the function which produced it caused the crash.

## v0.0.40 observation, not a proven crash repair

`ExceptionDiagnosticsEnabled=1` is the default even when absent from an older
INI. Before installing runtime hooks, the mod opens a 98304-byte file-backed
mapping at `scripts/FreeRoamRivals/NativeExceptions.log`. It pre-touches the pages,
captures the module inventory at installation, and registers a native vectored
exception observer. The ASI is pinned for the process lifetime to keep the
registered callback valid; unloading it in a running process is unsupported.

The callback observes access violations, in-page errors, illegal/privileged
instructions and integer divide/overflow exceptions. It writes bounded ASCII
records directly into the pre-mapped pages. It does not use the normal logger,
heap allocation, mutexes, stack walking, module lookup or file I/O in the callback.
It always returns `EXCEPTION_CONTINUE_SEARCH`, never changes the exception context
and never reads/writes game objects. Breakpoints, C++ exceptions and guard-page
notifications are deliberately excluded; stack-overflow handling is not attempted.

Records contain the native code, instruction/EIP, registers, exception parameters,
thread ID, sequence and a thread-local FRR callback phase. `outside_FRR_callbacks`
means only that no instrumented FRR callback is active on that thread, not that
FRR has been ruled out. Module ranges represent the installation snapshot; late
loads/unloads are not tracked. The latest 128 selected events are retained in a
ring, so sort by sequence rather than physical row. Busy concurrent ring slots
can be skipped. First-chance exceptions can be handled by the game and are not
automatically fatal. This observer cannot diagnose a normal exit, termination
without exception, an earlier crash, or faults excluded from its filter.

The file is replaced on each start. Save it immediately after an exit before
launching again. Windows tests cover both handled SEH exceptions and a real
unhandled PAGE_NOACCESS write in a child process, including observer propagation,
trace survival after fatal process termination, thread-local phase nesting,
teardown and ring overwrite. Those tests do not execute NFSMW.

## Next real-game check

1. Replace only `scripts/FreeRoamRivals.asi`; preserve the configured INI.
2. Move Bartender and X360Stuff ASIs outside the installation for an isolated test.
3. Start without pressing F8. If it exits, save both logs immediately.
4. If startup succeeds, enter ordinary free roam with the Golf GTI, obtain motion
   calibration, request F9 then F8 once, and capture the resulting factory stages.

The startup-exit cause, native GTI driving and safe pursuit retirement remain open.

Primary API references:
- https://learn.microsoft.com/en-us/windows/win32/api/winnt/nc-winnt-pvectored_exception_handler
- https://learn.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-addvectoredexceptionhandler
- https://learn.microsoft.com/en-us/windows/win32/debug/vectored-exception-handling
