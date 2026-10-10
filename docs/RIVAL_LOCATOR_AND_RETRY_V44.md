# v0.0.44-dev: locating a GTI and gated repeated tests

## Real capture used

In the 2026-10-10 log, the v42 process constructed the GTI at an initial relative distance of 74.4494 metres. It was activated and reported sustained autonomous motion before native observations became unavailable. The visual disappearance is not proof of destruction, despawn or intentional retirement.

In the later v43 process, an approximately 23.1498-metre candidate was constructed and identity-confirmed, then road navigation failed with road_reset_rejected. The source returned to cleanup; it waited for hidden/300m conditions. The native fault prevented further construction in that process. No native exception was identified as the cause.

## Read-only locator

The existing 6-row HUD now displays the straight-line 3D distance between the verified current player and mod-owned rival. Directions are classified as ahead, behind, left, right and four diagonals against actual horizontal travel direction; there is no claim of player orientation while stopped. No game minimap symbols, traffic controller writes or additional game hooks are involved. If the world or native rival identity is invalid, there is no old-pointer read to keep a fake marker alive.

## Configured controls

The play preset (config/FreeRoamRivals-play.ini) uses F8 to request one stock GTI spawn, G as the challenge fallback, and Ctrl+Shift+R held for 1.5 seconds to request retirement. The default INI leaves this retirement key disabled. The request is not instant: the car must be verified as mod-owned, hidden from current main camera with validated 8-ray occlusion, at least 300 metres distant, no active/uncertain pursuit, and the stable free-roam window must hold. Do not remove guards just for quicker testing.

On actual native cleanup, the owned object's pending Kill is observed until its identity is absent from BOTH live IVehicle and physical PVehicle registries on two distinct completed engine frames. Only then, if factory status is still not faulted/disabled and there is no owned/pending token, F8 is rearmed for a new conservative one-vehicle search. This is NOT automatic respawn.

On a road reset or construction fault, the factory deliberately remains disabled for this session even after cleanup. If streaming/wrong world context obscures identity, no implicit free/reuse is permitted. Relaunch the game for any new native creation after a fault.

## Road-reset instrumentation

A failure now includes roadResetDetail in NativeFactory fault logs. Possible classifications are vehicle_state_invalid, ai_missing_or_vtable_changed, road_seed_encoding_failed, native_reset_returned_false, post_reset_snapshot_invalid, post_reset_position_mismatch and native_seh_exception. These are observable subconditions inside the previously existing road reset operation, not a speculative ABI change. A failed native call still blocks further creation in that process.

## Test on the exact owner PC

1. Use the exact supported NFSMW v1.3 executable, stock Golf GTI and an isolated mod configuration; backup original INI and game profile. Use the play preset ONLY when intentionally testing native car creation.
2. Drive a little to calibrate metric movement. Tap F8 once in normal Free Roam. When active, check the distance HUD while stationary and the relative direction while moving around the car. Verify the direction changes; no minimap pin is promised.
3. Hold Ctrl+Shift+R and verify it requests cleanup, not a forced delete. Move at least 300m away; because the hidden/occlusion proof may remain unavailable, the cleanup can legitimately remain pending.
4. Only after a complete native removal is logged should F8 be retried. Ensure there was no earlier native fault. Observe that no duplicate car is admitted.
5. On road_reset_rejected preserve the new roadResetDetail, FreeRoamRivals.log and NativeExceptions.log. Do not bypass safety to force a repeat.

Windows CI tests the build and pure locator behavior, but does not establish target-game cleanup safety, navigation ABI correctness or streaming retention.
