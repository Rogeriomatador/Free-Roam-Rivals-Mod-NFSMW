# Spawn factory research

The v1/v2 conclusions are supplied Claude findings without original dumps, not
independently reproduced ABI proof. See POST_RACE_RACER_RESEARCH.md for further
public sources and a concrete roaming-racer implementation found on 2026-10-07.

Evidence gathered from the three SDKs pinned in CMakeLists.txt. Nothing here is
runtime-proven. The mod must not call any of these until the executable bytes
confirm the ABI (see `tools/dump_exe_functions.py`).

## What the SDKs claim

- `PVehicle::Construct(const VehicleParams&)` calls VA `0x689820` as
  `ISimable*(__cdecl*)(VehicleParams, UCrc32)`, passing `VehicleParams` BY VALUE
  plus its `mName`. Same wrapper in MWSDK and NFSPluginSDK.
- `VehicleParams` = `Sim::Param` header (`mType`, `mName`, `mData`, 4 pad bytes)
  followed by `mDriverClass`, `mVehicleKey`, `mDirection*`, `mPosition*`,
  `mCustomization*`, `mVehicleCache*`, `mPerformanceMatch*`, `mFlags`.
  The MWSDK header itself says offsets are "leads to confirm, not ground truth".
- `VehicleParams::AddTypeName` = `0x4040F0`; type hash `0x0A6B47FAC`.
- `IVehicleAI` exposes `SetDriveSpeed`, `SetDriveTarget`, `ResetDriveToNav`,
  `SetSpawned`, `UnSpawn`, `GetGoalName` (MWSDK/NFSPluginSDK `IVehicleAI.h`).
- `PVehicle::Kill()` is a virtual (PVehicle.h); `PVehicle::PVehicle` = `0x689020`.

## Red flags that block a blind call (status after exe dump v1/v2)

Status: items 2 and 3 are RESOLVED by docs/EXE_DUMP_V2_FINDINGS.md (engine reads
fields through mData; 0x689820 is cdecl with caller cleanup), as reported by Claude.
Item 1 remains ambiguous: the second SDK wrapper does not prove generic dispatch,
and the reported fixed type check suggests typed construction. Item 4 and the
compiled pointer-layout issue remain open.

1. **0x689820 is not vehicle-specific.** MWSDK `ResetCar.h` calls the same VA with
   `BehaviorParams`, and `Smackable.h` uses `0x6895A0` with `SmackableParams`.
   So it is probably a generic Sim object factory dispatching on the Param type.
2. **By-value struct in cdecl.** `Sim::Param::mData` is initialised to `this`
   (self pointer). A by-value copy keeps the pointer to the ORIGINAL object, so
   correctness depends on whether the engine reads through `mData`.
3. **Calling-convention/stack cleanup unverified.** The SDK `Sim::Param::GetData`
   uses odd pointer arithmetic, so its layout cannot be trusted blindly.
4. **No evidence yet** that `Racer` driver class construction works without stock
   race state (repo contract already notes this risk).

## Verification step (read-only, no game running)

`python tools/dump_exe_functions.py speed.exe` refuses any exe except the pinned
target (size 6029312, MD5 C0516B48...), then dumps 192 bytes at each VA, the
optional Capstone linear disassembly. Return candidates do not verify an ABI.
The tool refuses to overwrite the input or any existing report; choose a new
output filename for each capture. Use `--bytes 1400` for the reported v2 windows.
Send the generated `FRR_exe_dump.txt`; it is the input needed to confirm or
reject the ABI above before any construction code is written.

## Next steps (in order)

1. Confirm 0x689820 prologue/arguments/`ret N` from the dump.
2. Confirm how the engine reads the Param (via `mData` or fields).
3. Remaining spawn-candidate proofs: `WRoadNav` association for SeekAhead/FarFuture,
   region-loaded evidence, full-view invisibility.
