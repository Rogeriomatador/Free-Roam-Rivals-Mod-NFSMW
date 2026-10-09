# v0.0.40: visible GTI, cross-frame fleet check blocks AI preparation

## Real evidence

User capture: `FreeRoamRivals(20261009-193443).log`, SHA-256
`40bd842b423f7fdfc329ffea9c03d29ed5c53f7bc7eb5ff417d9e2d4b239eef7`.
The 2026-10-09 16:32:20.829 session reports v0.0.40. The uploaded INI
enables the native prototype and near-player target; runtime confirms both.

| Time | Observed log event |
| --- | --- |
| 16:32:36.246 | F9 native compatibility audit: checked=11, mismatches=0 |
| 16:33:23.270 | F8 requests seeking |
| 16:33:24.862 | Valid free-road target approximately 38.06 m away |
| 16:33:24.879 | Construction preflight again passes all 11 signatures |
| 16:33:24.914 | Constructor returns P=0x04710B30, IV=0x04710BDC, sim=0x04710B5C, handle=34; baselineLive=15, baselinePhysical=20 |
| 16:33:25.108 | `preexisting_registry_identity_lost`, phase=construction_confirmation, identity=not_checked, exception=0 |
| 16:33:25.109 | Prototype becomes disabled |

The factory never reaches confirmed ownership, Deactivate, Racer driver/goal,
road reset or Activate in this capture. The user reports that the visible car
only responds to gravity. This is consistent with stopping before AI setup;
physical motion does not prove AI activation or autonomous driving.

`NativeExceptions(1).log`, SHA-256
`6e9cb31535a4faa886e8800f29b502abec74e9847b5e39455d57f24af49c03f9`,
contains no FIRST_CHANCE records for the observer's selected exception types.
The reported factory stop is a policy fault, not a captured SEH exception.

The v40 log does not record the missing entry. Therefore it cannot determine
whether that entry disappeared inside the constructor or during a later game
update, nor identify the native reason. Counts alone do not establish which
identity was removed. Later runtime samples report different live population
counts; that is evidence of population changes, not a diagnosis of this entry.

## Source research and temporal scope

- [Pinned reconstruction AITrafficManager.cpp](https://github.com/dbalatoni13/nfsmw/blob/1f2cdd7996791c81a580b3f7b36b44d4f9f6719c/src/Speed/Indep/Src/AI/Activities/AITrafficManager.cpp): Update services spawning and validates active traffic; invalid vehicles call UnSpawn. FlushAllTraffic can UnSpawn or Kill. This is a WIP, multiple-platform reconstruction; it is an algorithmic reference, not proof of a particular PC registry mutation in this capture.
- [Pinned SDK PVehicle registry](https://github.com/berkayylmao/NFSPluginSDK/blob/d238bdffc648498840d17133edb256fa0310d355/NFSPluginSDK/Game.MW05/Types/PVehicle.h): physical entries use an instance pointer/enabled pair and null termination. The registry reader retains this representation, alongside the separate verified MWSDK live IVehicle table. No new registry address or guessed layout is introduced.
- Existing exact-executable capacity analysis remains in `NATIVE_FACTORY_ADAPTER.md`: the mod's synchronous construction guard excludes the native capacity-eviction branch. The 0x9377C8 physics count is not a count of race opponents.

Inference: the complete fleet snapshot is appropriate for a synchronous
operation's preservation check. Requiring every unrelated identity to survive
additional native updates is a separate global lifetime policy that does not
establish ownership of the new GTI. This release corrects that temporal scope;
it does not claim to know why the old entry disappeared.

## v41 behavior

1. Immediately after the one permitted constructor call, capture fresh
   registries before returning to the original loop. Reject unreadable
   snapshots or any missing previous identity. Log missing counts and first
   IV/P addresses with `scope=synchronous_constructor`.
2. Across completed frames, log the first unrelated fleet difference with
   `scope=completed_frame_observation`. The player and GTI must each be
   present in both current registries. World, road, race, profile and both
   player interfaces must still match the captured context.
3. In EACH member frame, check the GTI's IVehicle vtable, simable pointer,
   captured handle, model key, and exclusion of player/player-owned cars before
   any AI pointer use. Verify the player's pursuit/cooldown and the GTI's own
   pursuit are clear. Any uncertainty resets the confirmation streak; changed
   owned identity faults. No object read follows absent membership.
4. Require two distinct completed gameplay frames, with the existing two-second
   timeout. Confirmation runs before the search's 250ms throttle. Only after
   confirmation can deactivation, loading, Racer preparation, navigation and
   activation proceed. Existing guards on those stages and removal remain.

No global traffic hooks/goals, blind acceleration writes, signature bypass,
forced reactivation, unverified free/delete or repeat constructors are added.

## Validation and remaining game proof

Portable native safety regression passes for reordered/additional entries,
unrelated turnover between frames, synchronous loss rejection, absence of
each player/GTI interface, confirmation reset/recovery and alias/null refusal.
All 26 Python tool tests pass locally. The Win32 CI must compile and run the
20 registered CTest targets plus those Python tests before merging/releasing.
No NFSMW game session is executed by that CI or in this workspace.

Install only the v0.0.41 ASI, preserving the already-enabled INI. Restart the
game, enter ordinary Free Roam with the player's Golf GTI and no pursuit,
drive for calibration, F9 then F8 once on a clear road. The log should show
synchronousBaselinePreserved=1, two member confirmations, confirmed inactive
ownership, Racer/navigation preparation and activation if their guards pass.
Observe actual acceleration, steering and route following; displacement alone
could still be gravity or collision. Send both logs even if the car remains
stationary. If synchronous preservation fails, the new missing-entry evidence
is necessary; do not retry the spent constructor within that session.

Autonomous roaming, challenge behavior, pursuit interaction and safe retirement
are not proven by this correction and remain open game-validation tasks.
