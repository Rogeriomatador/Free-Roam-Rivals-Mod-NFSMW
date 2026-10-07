# Render observation recovery — v0.0.25

## Target report

The supplied log contains a v0.0.24 boot at 2026-10-07 11:57:34. The executable
guard passes for size 6029312 / MD5 C0516B485065FABDD69579816B5DF763. At the
8-second health check, renderFrames, inputPolls, frameTicks and samples are all
zero. There are no FRR_MOTION_V1 rows or later runtime snapshots in this file.
The offline auditor therefore returns no cohorts and exit code 1.

The user reports Carrera GT, then Cobalt SS, subsequent tuning and KPH HUD.
Those are operator notes, not captured measurements. We cannot derive scale,
car transitions or tuning effects from this log. The 8-second counters alone
also do not establish what happened at every later instant; the absent series
is why periodic health is now required.

## Confirmed implementation shortcomings

The pinned SDK's d3d9_hooks.h waits for the first non-null device and replaces
one EndScene vtable slot. Its worker does not check the vtable-write return,
does not follow later devices/vtables, and provides no Present fallback. The
old runtime reports the asynchronous request as armed without confirmation
that its callback reached the sampler. Its health thread reports only once.

These shortcomings explain possible failure paths; the supplied log cannot
identify which path occurred. Input-poll delivery is also unproven. This release
does not substitute render callbacks for confirmed gameplay/input callbacks.

## Revised bridge

- Read the executable image and device fields with guarded ReadProcessMemory.
- Resolve the device global from the MW05 Reset call-site shape independently
  used in WidescreenFixesPack; require a global within the guarded image and
  reject conflicting matches. If absent, retain the already pinned SDK global
  0x982BDC and label that fallback explicitly. No arbitrary offset is selected.
- Require readable device/vtable and executable EndScene/Present targets.
- Hook method entries with MinHook, retaining a distinct trampoline for each
  implementation. This also intercepts previously cached method pointers.
- Publish the original trampoline before enabling each detour; forward the
  complete COM signature and original HRESULT. No device vtable is overwritten.
- Follow live device/vtable changes from a worker that reads only render
  pointers/code. It never samples gameplay objects or runs world collision.
- Retain at most four implementations per method; never replace a trampoline
  that an older device can still call. Unsupported/ambiguous evidence fails closed.
- Use EndScene as primary. Present supplies observation when that device has
  no EndScene or the last EndScene is over 500 ms old. A current EndScene suppresses
  duplicate Present sampling; a device change resets that lease.
- Serialize sample delivery without blocking concurrent render callbacks.
  Old devices still chain their original methods but cannot sample a new world.
- Log method installation status, module paths, device/global/vtable and raw
  callback counts. Log the first delivered runtime frame, actual INI path and
  loaded diagnostic switches. Repeat health after 8 seconds and every 30 seconds.

## Provenance

- Pinned [SDK d3d9_hooks.h](https://github.com/s-b-repo/nfsmw-2005-sdk/blob/3b3d05b9194844883aa42d75dd7aae66838092ff/include/nfsmw_sdk/d3d9_hooks.h):
  device global and COM slots (EndScene 42, Present 17).
- [WidescreenFixesPack MW05](https://github.com/ThirteenAG/WidescreenFixesPack/blob/2b34384ddc038c99b730a993d6a793adfd855eaf/source/NFSMostWanted.WidescreenFix/dllmain.cpp):
  resolves Direct3DDevice from the Reset call-site AOB. The bridge additionally
  validates MOV ECX,[EAX], Reset slot 0x40 and image bounds.
- [RE render notes](https://github.com/s-b-repo/nfsmw-2005-re/blob/0dbc9393e263d95d182e6c98bff3dab4140aa2ff/notes/project_render_pipeline.md):
  corroborating device global and standard COM slots. This shares SDK provenance
  and is not counted as an independent runtime proof.

## Validation and remaining limitation

Portable tests cover discovery shape/bounds/ambiguity, fallback expiry, clock
regression, device replacement and duplicate suppression. Native Win32 tests
exercise real MinHook trampolines on COM-like test objects, cached entry calls,
argument/HRESULT forwarding, separate implementations and old-device chaining.
They do not establish target-game success. A fresh log must show increasing
renderFrames/samples and FRR_MOTION_V1 rows before driving data can be reviewed.

FrameTick, input-thread matching, metric, streaming, complete visibility and
construction gates are unchanged. Zero input callbacks still block gameplay
queries/mutation even if Present restores motion capture.
