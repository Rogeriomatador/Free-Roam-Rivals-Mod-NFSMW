# Ghidra native address boundaries — 18 Oct 2026 (owner-supplied ZIP)

This evidence was obtained from the user-owned ZIP
`ghidra_native_boundaries_20261018-135645-932758.zip`, produced by the
read-only `tools/ghidra_native_boundary_audit.py`. The ZIP itself contains
proprietary bytes, stays **local/private**, and is NOT checked into GitHub.

## Context and interpretation

- Game EXE on disk: 6,029,312 bytes;
  MD5 `C0516B485065FABDD69579816B5DF763`.
- Ghidra reports PE32 x86, base `0x00400000`, `speed.exe` loaded from
  `D:/Need For Speed Most Wanted Black Edition/speed.exe`.
  Its imported image has **not** been independently hash-checked by the bridge.
- Query result: **12 semantic endpoint errors**, exactly **2 each**
  for six addresses: `get_function_by_address` and
  `disassemble_function` returned **no analyzed Ghidra function**.
  The **`read_memory` GET did return 32 bytes for all twelve
  addresses**, and `get_xrefs_to` returned usable references.
  This is **missing Ghidra function recognition**, not proof the
  address is invalid, empty, or inaccessible.
- Independently decoded the returned 32-byte x86 windows as **static
  research evidence** (do not conflate instruction decoding with
  an independently verified callable ABI).

## Six unresolved Ghidra function entries: instruction evidence

| VA | Incoming DATA reference | Evidence from first 32 bytes | Interpretation |
| --- | --- | --- | --- |
| `0x00689820` | `0x008875C7` | Reads SEH head from FS, establishes frame, allocates local stack, saves registers | plausible constructor/reset entry, **full boundary + ABI unknown** |
| `0x006851D0` | `0x008AA948` | saves ECX/ESI/EDI; sets object byte `+0x1C` to 1; calls `0x00676540`; checks another field | consistent with retirement/destroy-related path, **not proof of immediate destruction** |
| `0x00688120` | `0x008AA888` | `FLD [ECX+0xD4]; RET` followed by `INT3` padding, with another float getter at `0x00688130` | strong single-return getter boundary hypothesis |
| `0x006693A0` | `0x008AA8A8` | checks `[ECX+0xAC]`; if zero calls virtual `[vtable+0x34]` with argument `1`; returns before padding | strongly consistent with an activation wrapper, not proof of semantic meaning of `+0xAC` |
| `0x006693C0` | `0x008AA8AC` | calls virtual `[vtable+0x34]` with argument `0`; returns and pads; next routine starts at `0x006693D0` | strongly consistent with deactivation wrapper |
| `0x006876E0` | `0x008AA87C` | reads first stack arg, compares against `[ECX+0x94]`, writes field if changed and calls `0x0067ECA0` | strong setter pattern; **tail beyond 32 bytes is missing** |

### Vtable offsets (hypotheses grounded in incoming data xrefs)

FRR's IVehicle vtable reference `0x008AA828` yields:

- `0x008AA87C` = `+0x54` (index 21) -> `0x006876E0`;
- `0x008AA888` = `+0x60` (index 24) -> `0x00688120`;
- `0x008AA8A8` = `+0x80` (index 32) -> `0x006693A0`;
- `0x008AA8AC` = `+0x84` (index 33) -> `0x006693C0`;
- `0x008AA948` = `+0x120` (index 72) -> `0x006851D0`.

These are offsets from a historically supported vtable base,
**not** an independent validation of all slot names, inheritance
relationships, object layout, or `this` adjustments. The DATA
xref at `0x008875C7` for `0x00689820` is not aligned like the
others and cannot be assumed to be an IVehicle vtable slot.

## Disputed address 0x0043D388: resolved interior (NOT standalone function)

**Ghidra `get_function_by_address(0x0043D388)` identifies**
`FUN_0043D330`, Entry `0x0043D330`, body
`0x0043D330–0x0043D3FD`. The function listing includes:

- an instruction **at** `0x0043D388`, namely a `PUSH` that occurs
  **after** subobject initialization at `0x0043D382`, and
- further writes of the goal vtable and AI sub-vtables.
- **No inbound xref** to `0x0043D388` itself was found.

This is affirmative evidence that `0x0043D388` is an **interior
instruction boundary**, *not* the entry of a separate
`CreateAIGoalRacerInstance` function in the analyzed Ghidra program.
Calling it as a constructor would bypass initialization, including
the SEH setup. DO NOT CALL OR HOOK IT.

By comparison:
`0x0043D330, 0x0043D400, 0x0043EF70, 0x0043F040,
0x00422480` all had Ghidra function metadata, listing and valid
memory reads. This demonstrates the missing-function responses of the
other six are not a blanket MCP transport failure.

## Implications for FreeRoamRivals

**Neither a blank Ghidra function record nor an xref to a vtable is
permission to call a native address from the game plugin**.
FRR's `NativeVehicleFactory.cpp` already has 11 target function
fingerprint checks and guards. Keep those checks, fail-closed defaults,
single-owner guard and SEH handling. New byte evidence does
**not** authorize changing `0x006851D0` (Kill),
`0x006693A0` (Activate), `0x006693C0` (Deactivate) or
`0x006876E0` (driver setter) semantics or adding additional hooks.

The last live Rico session observed a changed
`ISimable::_mHandle` while still listed in both physical/native
registries. **This static audit neither identifies the writer nor
proves a cause of handle change**. We added read-only
`expectedSimableHandle` / `observedSimableHandle`
telemetry to narrow subsequent game logs while never rewriting
the owner token or forcing respa​wn.

## Next safe proof

1. Confirm on the *same supported executable* the full
   boundaries/disassembly for `0x00689820`, `0x006851D0`,
   `0x006876E0`. Ghidra auto-analysis may omit valid, indirectly
   referenced code; do not repair analysis by patching the EXE.
2. Verify callers for these vtable slots and the `this` context,
   using exact code bytes and matching version before changes.
3. Trace *writers* and release/streaming events for
   `ISimable::_mHandle` instead of equating missing AI tick
   evidence with the observed lifecycle bug.
4. Correlate repeated in-game native presence-loss telemetry with
   handle samples and both registries. No uncontrolled Kill,
   forced respawn or owner-token retarget.

All of this remains **research-only**; no claim that Rico's
disappearance is fixed.
