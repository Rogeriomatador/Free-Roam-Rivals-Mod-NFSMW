# Exe dump v1 findings (speed.exe, MD5 C0516B48...)

Correction from direct executable inspection on 2026-10-08: 0x43D330 installs AIVehicleRacecar primary/AI vtables (0x892720 / 0x892640 at +0x4C). It is NOT an AIGoalRacer constructor or CreateAIGoalRacerInstance. Historical labels below are superseded. All eleven v2 1400-byte hashes were independently reproduced against the user upload.

Provenance: imported Claude report; original dumps were not supplied with these
patches. No independent local reproduction or runtime execution is claimed.

Source: `FRR_exe_dump.txt` from `tools/dump_exe_functions.py`, captured on the
target machine. Disassembled offline with capstone (x86-32). Static evidence only;
nothing was executed. Only the first 192 bytes of each function were dumped.

## Confirmed by bytes

1. **0x689820 reads VehicleParams through `mData`.** After a type-hash check
   (`cmp eax, 0xA6B47FAC` on the dword at entry+8) it loads `eax = [param+8]`
   (mData) and then reads dwords at `+0x10,+0x14,+0x18,+0x1C,+0x20,+0x24,+0x28,+0x2C`.
   That is exactly the 8-field tail of the SDK `VehicleParams` (DriverClass,
   VehicleKey, Direction, Position, Customization, VehicleCache, PerformanceMatch,
   Flags). So the SDK layout and the self-pointer `mData` are real; a by-value copy
   works only while the ORIGINAL VehicleParams object stays alive and mData points to it.
2. **Stack-argument function, not thiscall.** 0x689820 has an SEH frame, 0xA0 bytes
   of locals and reads its arguments from the stack; ecx is not used on entry.
   Callee stack cleanup (`ret` vs `ret N`) and the position of the second
   `UCrc32` argument are NOT visible in 192 bytes. Still open.
3. **0x43D388 is not a function entry.** It lies inside the function that starts at
   0x43D330 (`ret 4` at 0x43D3FB). The next function starts at 0x43D400, allocates
   0x7CC bytes from a pool at 0x925B30 and calls the constructor. Repo docs listed
   0x43D388 as CreateAIGoalRacerInstance; corrected to 0x43D330. No source file used
   that address, so no code was affected.
4. 0x43EF70 is a real entry (SEH prologue, thiscall, one stack arg) and looks like an
   AIVehicleRacer constructor that installs vtables, not a pool-allocating factory.
5. 0x6880B0 is a table of tiny getters (`mov eax,[ecx+0x94]; ret` first), consistent
   with the MWSDK-verified driver-class slot.
6. The heuristic `firstReturn` for 0x43EF70 is a false positive (`3b c3` contains 0xC3).

## Still unproven (blocks any construction code)

- `ret` / `ret N` of 0x689820 and where the `UCrc32` argument sits.
- Which later branch handles `DriverClass::Racer` and what state it needs.
- What 0x4E4EA0 (called at 0x6898BC) does, and whether it needs stock race state.

Next dump: `python dump_exe_functions.py speed.exe --bytes 1400` (covers 0x689820 to its
return and the callees listed above).
