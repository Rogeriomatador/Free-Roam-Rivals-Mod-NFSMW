# Exact-target executable evidence — 2026-10-08

## Provenance and method

User uploaded speed.exe directly. Read-only PE32 mapping and GNU objdump x86 Intel disassembly were performed; the executable was never run, edited or redistributed. Original game bytes and disassembly listings are not included in this repository.

Size: 6029312 bytes. MD5: C0516B485065FABDD69579816B5DF763. SHA-256: 80774c2e5d619b4f120b48d4462896fd504c263399d203a238769cffde1d253c. This matches FRR's pinned target.

All eleven 1400-byte SHA-256 windows published in EXE_DUMP_V2_FINDINGS.md match the uploaded executable. Those hashes are now independently reproduced; broader runtime conclusions are not thereby proved.

The PE Characteristics value is 0x10F, without LAA. Setting only bit 0x20 in an in-memory copy produces SHA-256 BDE12BDD158B7F861078AD4527F5A656F34C6068E4A2120F28044F92F0FA158C, not NFRR's documented B248271B... target. The original is untouched. This demonstrates that the difference is not exclusively that bit; it does not identify every difference or prove function-level incompatibility.

## Directly inspected instructions and static vtable slots

| Entry | Observed behavior / scope |
| --- | --- |
| 0x689820 | Vehicle-specific type hash check; reads payload through Param mData; plain return at 0x689CCA. Factory ABI remains subject to exact struct and caller validation. |
| 0x689C29 | Successful factory result is object + 0x2C (ISimable interface). Do not treat it as IVehicle or double-adjust an SDK cast. |
| 0x6876E0 | ECX is IVehicle, driver argument is on stack; writes +0x94, calls attachment rebuilding routines using IVehicle minus 0xAC; ret 4 at 0x687708. |
| 0x6693A0 / 0x6693C0 | Activation/deactivation thunks use ECX and invoke virtual slot +0x34 with boolean 1/0; no explicit caller stack argument. |
| 0x422690 | AI interface in ECX, one road argument; calls 0x777660 with source and mode zero; ret 4 at 0x4226D5. Returns a boolean from its vehicle reposition operation; caller must evaluate it. |
| 0x777660 | Road-navigation initialization includes graph-dependent work and subordinate calls; this is not authorization for arbitrary memcpy or an invalid road source. |
| 0x422480 | Primary AI in ECX, reference/pointer to goal hash on stack; reads through it, compares/writes +0xC4, destroys existing +0xB8 goal through virtual call and invokes registered goal factory; ret 4 at 0x422604. Goal changes are mutations, not safe observation. |
| 0x42B070 | Goal in ECX, one stack value forwarded to action methods; selects/replaces current action at +4; ret 4 at 0x42B11F. Float meaning is consistent with SDK/source, but type is not proved solely by forwarding a DWORD. |
| 0x6851D0 | ISimable in ECX; writes retirement flag +0x1C, uses object minus 0x2C and removes registry links; plain ret at 0x68523E. This is not proof of synchronous destruction or pointer lifetime after return. |
| 0x5E8A20 | Cache query uses ECX and two stack arguments; branches include ret 8. Existing-vehicle cache protection remains an independent implementation requirement. |

Static data checks: IVehicle vtable 0x8AA828 slots 2/21/22/32/33/43 point to 0x688030/0x6876E0/0x6880B0/0x6693A0/0x6693C0/0x688230. Racer AI interface vtable 0x892640 slots 22/34 point to 0x422690/0x415D00. Racer goal vtable 0x892D30 slot 1 points to 0x42B070. These are on-disk values, not a guarantee that any live pointer is of that type.

## What this unlocks and what it does not

The missing-executable blocker is resolved. The adapter can now be developed against independently inspected target code instead of treating public-source addresses as sufficient evidence.

Still required: compiled SDK interface-layout validation; valid cache and customization record preparation; fresh moving-anchor identity and native road initialization; construction replacement protection; exact pursuit-state interpretation; owned-identity cleanup and retirement completion; a user gameplay run showing one newly created rival actually driving. Existing diagnostics do not create that rival. No native creation was enabled by this report.

## Reproducible 192-byte windows

The hashes below include padding or following code when the function is shorter; these windows do not assert function lengths or ABI verification.

| Label | VA | SHA-256 of 192 bytes |
| --- | --- | --- |
| PVehicle::Construct (generic Sim factory) | `0x689820` | `10996cc5eac53388cb79fec36a271809c51cf384af5c662fc96b70463a18592c` |
| PVehicle::PVehicle | `0x689020` | `02ca3a9a29ac9a38c237ae1c8ba6e579b20c72af0ef2ea1d9c2873d5f1a71701` |
| IVehicle::GetDriverClass | `0x6880B0` | `75f8c6f62f52fe23db1fd4b416983271094e0e926900adf232ec65ef6d397303` |
| VehicleParams::AddTypeName | `0x4040F0` | `797f6bb315cf7fc963b5f3a41fb65a04a6ea494c834d840d83cb5960910d3e0d` |
| Smackable factory (comparison) | `0x6895A0` | `55244f0cfdf5f59142d4b48e66fc1e21e10db5378760837c47ac82b919d673db` |
| CreateAIVehicleRacerInstance | `0x43EF70` | `4002b9002dc71a41da82764e227a1b47181e84b92d0258d751f9cedb2949a6eb` |
| AIGoalRacer constructor entry (reported) | `0x43D330` | `3f69edd28728364cb334f2933b08b8778288da0788d0e96a0b4a09d7b198d6c8` |
| AIGoalRacer interior address (do not call) | `0x43D388` | `e809d2be3aa2dc1b4610866e2d99bb15f5e857caf2c00157d5b2b2c52746e7ea` |
| AI goal racer pool wrapper (found in dump v1) | `0x43D400` | `1910fc62cbc145566535bc89ba4f5e69030f4248b1f4fa5b7325fd6d36aa8953` |
| Object creation callee of 0x689820 | `0x4E4EA0` | `84ee6465f09f39eaf9b9f7fd0a0eea5289bd646b600de535caeaec6af8cda3b5` |
| Activate vehicle | `0x6693A0` | `d4c0ef74a3abb6abda224357cae6960f1c58236347af546a924ac0f55cb437a4` |
| Deactivate vehicle | `0x6693C0` | `6ba95634bf2347239632ea95ef8043f5766d50aee73462fc6206b418bdee1f3d` |
| SetDriverClass | `0x6876E0` | `bb63237c12dea772cb4b32153cbdb835f6c9886179656d506a06ebac2fabc83b` |
| AI ResetVehicleToRoadNav | `0x422690` | `ae554dc9631e3f36a32f1d256a8a298ffe068e182bf7f3da45fd8cc0c72f1862` |
| AI SetSpawned | `0x415D00` | `7606c5e00439440c8ce4a763a2c5dd390f2dbc16f5e1e5daac8e11b8a270677f` |
| AIVehicle SetGoal | `0x422480` | `30b47e2c642c8b50826f567cd9627e2f9048b7b9ca1115cc5815febbf23e2e35` |
| Racer goal ChooseAction | `0x42B070` | `a5506d508087e2f31c6fea23f4f18e46ba7ca9d966073329ced001d9d3e7369b` |
| KillSimable retirement entry | `0x6851D0` | `eb44ccbea4a939faa673843f2d54b339c70ab9249e8002a31123f0472b1da6a2` |
| Vehicle off-screen time | `0x688120` | `cf07553269a37d1c4f326151ed70d54f1da35ca1112a013e80e37f1de6404f1d` |
| Param type-check failure callee | `0x45CD20` | `00ef972b7fefb7e1ac701e0536c63387ad1404634902d2b849fd3e2de79c47ff` |


## Win32 compiler layout evidence

MSVC CI run 37746665390 emitted PVehicle size 352 (0x160), ISimable base +44 (0x2C), IVehicle base +172 (0xAC), mAI +256 (0x100), driver +320 (0x140). All VehicleParams and member offset assertions passed. The initial full-size assertion failed because this SDK describes a prefix, not the 428-byte native allocation. The corrected test explicitly checks the known 0x160 prefix and forbids conflating SDK sizeof with engine allocation size. Verify the final rerun before considering the build accepted.
