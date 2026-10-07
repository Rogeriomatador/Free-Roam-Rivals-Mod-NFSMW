# Exe dump v2 findings (full 0x689820 and callees)

Provenance: imported Claude report. Original dumps/executable were not included
with the patches, so disassembly/hashes have not been independently reproduced
here. "Confirmed" below is the original report's classification, not permission
to call engine functions. The generic-factory label is disputed: a second SDK
wrapper at the same address is not proof of generic dispatch.

Source: `FRR_exe_dump.txt` produced with `--bytes 1400` on the supported speed.exe
(size 6029312, MD5 C0516B48...). Offline x86-32 disassembly with capstone. Static
evidence only; nothing was executed.

## Confirmed

1. **0x689820 is cdecl-compatible with caller cleanup.** The function exits at
   0x689CB2 -> `add esp,0xAC; ret` (0x689CCA), plain `ret`, no `ret N`. Bytes after
   0x689CCB are int3 padding and a different tiny float function (0x689CD0).
   So passing extra stack arguments (the SDK's by-value 0x30-byte VehicleParams plus
   a UCrc32) cannot unbalance the stack.
2. **Only the first 16 bytes of the by-value argument are read.** Entry reads are
   [E+4] (mType), [E+8] (mName == 0xA6B47FAC) and the address of the argument; the
   payload is then read through `mData` ([param+8]) at +0x10..+0x2C. The second
   `UCrc32` argument is not read in the function body. (Inference from the entry
   reads, not a proof that nothing else uses it.)
3. **mType must be valid or the game crashes.** 0x689820 compares mType against the
   value produced by 0x4040F0 (AddTypeName). On mismatch it calls 0x45CD20, which is
   just `int3; ret`, then loads `[0+8]` (null dereference). The SDK ctor calls
   AddTypeName, which writes mType (`mov [ecx],eax` in 0x4040F0). Any construction
   code MUST build VehicleParams through the SDK constructor, never memset/hand fill.
4. **Vehicle lookup fails safe.** 0x4E4EA0 is `ret 0xC`, thiscall: it looks up an
   attribute collection by class hash 0x4A97EC8F and the vehicle key. If it is empty
   the factory jumps to 0x689C9C and returns NULL (no crash for an unknown key).
5. **Native allocation path.** Factory allocates 0x1AC bytes from pool 0x925B30
   (0x5D29D0) and calls `PVehicle::PVehicle` (0x689020, thiscall, stack args).
   DriverClass (+0x10) is forwarded into that constructor; `test edi,edi` shows
   DriverClass 0 (Human) is treated specially. Racer is just a driver class value,
   not a separate factory.
6. **Returned pointer is PVehicle + 0x2C.** At 0x689C29 `add esi,0x2C` before return.
   The SDK wrapper does `static_cast<PVehicle*>(ISimable*)`, which is only right if the
   SDK class layout puts ISimable at +0x2C. A C++ static_cast already adjusts a
   non-virtual base pointer using the compiler's layout. Do not subtract again
   from that result. Validate the pinned SDK's compiled x86 layout and resolve
   both registries independently before use; no manual conversion is authorized.
7. Flags in VehicleParams (+0x2C) are tested bitwise (1, 2, 4, 8, 0x10) inside the
   factory, so the SDK default (SnapToGround | CalcPerformance) is meaningful.

## Still unproven

- What the PVehicle constructor does for DriverClass::Racer and which AI/behaviors
  attach automatically.
- Whether `mVehicleCache == nullptr` is safe for Racer (the SDK default is null).
- Cleanup path: IVehicleAI::UnSpawn and PVehicle::Kill ordering (not dumped yet).
- Spawn candidate evidence is unchanged: remote road association, streaming and
  full-view invisibility remain open, so construction stays disabled.

## Reproducibility (sha256 of the first 1400 bytes of each function)

Raw bytes are intentionally not stored in the repo; compare hashes from
`tools/dump_exe_functions.py --bytes 1400` on the supported speed.exe.

| VA | Role | sha256 |
| --- | --- | --- |
| 0x689820 | generic Sim factory (PVehicle::Construct) | ac307525c169c70ce2e9864d418e71784201cf754e8b384e892c6fa7bf2c3717 |
| 0x689020 | PVehicle::PVehicle | 15c45a3598f58d7d88e06f12e3f11f007bbcbbd46a6676c58eb25cff3763503f |
| 0x6880B0 | IVehicle getters table | d990eee34d63dee551731eddc00b4248ed76bb476798a2d94274cc00f834ed02 |
| 0x4040F0 | VehicleParams::AddTypeName | 6500a20621552e422a1df4a0f68aef305d66f6ed79e6d6cb0c7827b2ef40c802 |
| 0x6895A0 | Smackable factory | ff88c9adf2757bdfa6deb65d4d2b694446e7f1dcafd2aac57001368aab925737 |
| 0x43EF70 | AIVehicleRacer constructor | 6111831f2eb4bbcad9c788ee619e7c4015d0ba6860764051e32c8105a06f9b9f |
| 0x43D330 | AIGoalRacer constructor (real entry) | 384a29d3d2137e03610f27b46ba5605734052993dcb0234954e432dcfab0d41f |
| 0x43D388 | interior of 0x43D330 (NOT an entry) | e560df571587591e15c5a141ca938a37d9ef62e4d27b856705e4b94c17ccef72 |
| 0x43D400 | AIGoalRacer pool wrapper | bc43ae2d060be9b36c6f4cf6dae8474605e08e4d80f8edd6a937ca295f1fc941 |
| 0x4E4EA0 | attribute lookup (class 0x4A97EC8F = pvehicle per MWSDK db) | a905367e9b3bc34cc85c188cff0352981fdea0c142ccfc28b77ef7284af22560 |
| 0x45CD20 | type-check failure stub (int3; ret) | 0e5ec0f6e4a9ad56eeed5e5de08fecd7e987fd146a991cffe6536ee645fde55c |
