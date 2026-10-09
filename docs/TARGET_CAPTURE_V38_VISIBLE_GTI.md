# v0.0.38: visible Golf GTI, activation not reached

## Real capture

User supplied `FreeRoamRivals(20261009-072727).log` (SHA-256 `78b111fba3b14c1b86c7872e699e3322a61d88d136994b93a6e503facfbf6ef2`). The file contains two sessions. The 04:20 session still loaded Bartender and rejected SetGoal; do not mistake it for the new 04:24 session.

The user explicitly reported seeing the new GTI stationary. This is user visual evidence, not an automated render test. At 04:25:01 and 04:25:51 the later session checked all eleven unchanged function signatures: zero mismatches. At 04:25:51.986 it logged `NativeFactory constructor invoke`, immediately followed by `stage=disabled`. No loading, Racer preparation, road preparation or activation stage was logged. This is not proof of native AI driving.

The v38 code can return Faulted after a constructor exception/null result, preexisting/missing registry identity, identity rejection, or deactivate/stamp failure. The capture did not report which branch. A visible object does not prove that the factory returned normally or that ownership was accepted.

The post-race read-only observer reported an incomplete sample immediately after construction and stable samples later. Its incomplete status can include a vehicle that is still loading; it does NOT prove the factory registry read failed. A transient readiness problem is therefore a hypothesis, not an established cause.

## Bartender evidence

Earlier sessions show an E9 at 0x4224B0. The recorded displacement leads to 0x71D9A310, inside the logged Bartender module [0x71D90000, 0x71DF1000). In the final session SetGoal matches the original file again. This identifies the conflict for the supplied runs; no whitelist or patch bypass was added.

## v39 implementation

A successful native return is retained as a pending scalar identity. No ownership-dependent virtual reads or writes occur until the object is present in BOTH fresh registries in two distinct completed gameplay samples, in the original world/profile and with a clear pursuit state. The preconstruction identities must remain present. Confirmation has a two-second wall-clock limit. Incomplete reads, absent membership, changed context or uncertain pursuit reset the streak. A constructor is never retried.

After confirmation, the original strict vtable/simable/key/non-player checks still run. Only then does the mod record ownership, deactivate the car and stamp its handle. Racer conversion, navigation reset, SetSpawned/goal/Activate and removal retain their existing guards. Pending confirmation is not a movement fix proven in the game.

Native failures now report phase, identity reason, expected/observed key, pointer diagnostics, SEH exception code, fault instruction and access address when available. A constructor fault may leave a partial native allocation: the mod stops, does not adopt an unreturned pointer, and does not invent a free/delete or a second spawn. Restart is required after a disabled partial construction.

The SDK ABI test checks numeric base conversions PVehicle -> ISimable +0x2C and IVehicle +0xAC without constructing/dereferencing game objects. Portable tests exercise repeated frames, absent membership, unreadable registry and pursuit interruption.

## Research and limits

- [Pinned NFSPluginSDK PVehicle factory](https://github.com/berkayylmao/NFSPluginSDK/blob/d238bdffc648498840d17133edb256fa0310d355/NFSPluginSDK/Game.MW05/Types/PVehicle.h): wrapper returns ISimable then converts to PVehicle. The supplied executable confirms its factory success path adds +0x2C and returns that interface; IVehicle getter returns primary +0x2C from IVehicle +0xAC. SDK layout is still checked by the Windows compiler.
- [MostWantedRacerAITraffic source](https://github.com/gaycoderprincess/MostWantedRacerAITraffic/blob/main/main.cpp): changes traffic's driving goal and compensates for drive-speed behavior. It is a different integration path; setting a goal alone is not proof of movement. No code imported and no global traffic mutation added.
- [Bartender](https://github.com/rng-guy/NFSMWBartender): pursuit configuration and engine hooks. Compatibility remains an open task; keep it unloaded for this prototype test.
- Existing engine ownership, ground, occupancy, profile and pursuit research is in `NATIVE_FACTORY_ADAPTER.md`, `NATIVE_RIVAL_PROTOTYPE.md` and `DIAGNOSTIC_RESEARCH_2026-10-09.md`.

## Next game test

Replace only scripts/FreeRoamRivals.asi with v0.0.39-dev, preserving the configured INI. Keep Bartender unloaded. Drive the GTI for calibration, F9, then F8 once on a free road outside pursuit. Observe whether the car drives. Send the log even if it stays still. If the native constructor faults, do not press F8 repeatedly; the exception and phase now define the next executable investigation. Police interaction and safe removal are still unverified.
