# v0.0.57-dev — Rico disappears at ~7 m: read-only diagnosis

## Evidence from real game log (2026-10-11 22:04:44–22:04:55 local)

The user ran `v0.0.56-dev` and pressed G at 22:04:44.600. The actual engine log says:
`LiveEncounter started: mode=Outrun_only ... sideBySideStart=0 jointCountdown=0 ...`.

The copied INI had `[Outrun] Enabled=1`, `[Staging] Enabled=0` and **no** `NativeDuelPilotEnabled=1`. Consequently **none** of the experimental `ForceStopOn/Off` native writes occurred. The run was the previous follow-the-leader Outrun; v56 cannot be credited with physically braking either vehicle in this test.

For ~10 seconds the rival's recorded lead fell from 29.65 m to 1.84 m. At 22:04:55.091 the last healthy physical sample saw Rico **7.04335 m** from the player and moving **56.0771 m/s (~202 km/h)**. At 22:04:55.351 the snapshot became unavailable **in the same Free Roam context**, not at a far-distance threshold. **Both registries still contained the numeric IVehicle/PVehicle owner**, with liveCount=20, physicalCount=37. The observed racer count then fell from one to zero while the vehicle registry count remained 20. No fatal exception was identified by NativeExceptions.

`battles=7 wins=0 losses=0 aborts=7` after the interruption. The result was aborted with `rival_native_presence_lost`; it was not a loss or successful finish.

## Diagnosis limits

Membership in both native registries establishes only that the numeric owner has *not* been confirmed retired. It does not prove that a usable rigid body, spooled model, Racer driver/goal or valid vehicle physics still exists. Merely calling this “streaming because distance” overstates the evidence; loss happened when the car was within 7 m.

## New bounded instrumentation

- `NativeOwnedSnapshot::availabilityReason` distinguishes an unsafe world, changed context, unreadable registries, identity-check stage (`IVehicle_vtable`, `simable_pointer`, `simable_handle`, `vehicle_key`, etc.), missing rigid body, invalid physical orientation/dimensions, nonfinite speed, or native read exception.
- Only **after both registries and native identity are verified**, read the existing `GetDriverClass()` and `IsSpooled()` methods (read-only), plus active/loading/destroyed flags; add these to the **one-off** near-presence-loss warning. The new probes do not hook or alter the lifetime, speed or driver class.
- Change the repetitive warning to say **physical snapshot unavailable, cause NOT established**. Do not force-delete, duplicate spawn or modify pointers when an owner is registered but unavailable.
- HUD now honestly says `[G] OUTRUN: SEGUIR RICO (SEM LARGADA)` when that mode is armed, so G is not misrepresented as a true Sprint.

## Next validation

Close game, install the ASI-only package (preserves current INI, WidescreenFix, saved rivals and world-metric cache). Keep `NativeDuelPilotEnabled=0` for the investigation. Reproduce F8 and follow G if safe, then send `FreeRoamRivals.log` and `NativeExceptions.log`. The single `NativePrototype sudden native presence loss` entry now includes `ownerReadReason`, `driverClassObserved`, `nativeDriverClass`, `spooledObserved`, `nativeSpooled`, `nativeActive`, `nativeLoading`. Use the results to decide whether an intervention is possible, rather than guessing.

The actual auto-stopping/alignment and scored Sprint are **still not implemented**. v57 is a diagnostic step, not a fix for sudden disappearance.
