# Guarded native factory adapter (2026-10-08)

## Deliverable and limits

`src/game/NativeVehicleFactory.cpp` now implements native construction, immediate
inactivation, racer driver conversion, native Racer goal selection and native
retirement requests. It is compiled into the plugin's source target. **RuntimeProbe
has no call to this adapter. It cannot currently create a rival for the player.**
No activation entry point is exposed yet. The remaining integration must provide
an owned, initialized road-navigation seed, prove the candidate remains hidden and
clear, activate on the verified gameplay thread, observe native movement and
exercise delayed retirement in the actual game. No game execution is claimed.

This is independently implemented from the uploaded executable and the pinned
BSD-licensed SDK; no implementation or binary from NativeFreeRoamRacers is copied.

## Construction contract

- One owned native object maximum. Existing SpawnSafety environment/candidate
  decisions must permit the request. No borrowed cache, customization or performance
  object survives the call. Position and normalized forward vectors are local.
- File identity must match the supported executable. The first 256 loaded bytes of
  six native entries must also match independently derived FNV-1a fingerprints;
  these are compatibility checks, not cryptographic authenticity proofs.
- All access is restricted to the installed, verified, consistent gameplay thread.
  Current player/profile/road/race context must still identify roaming without
  loading, NIS or fade.
- A fresh player registry entry and the IVehicle vtable are checked before resolving
  its current AI. AI slot 40 must target the unmodified getter at `0x431D70`;
  it returns `[IVehicleAI + 0x70]`, the pursuit pointer. Any non-null pointer or
  `GRaceStatus::mPlayerPursuitInCooldown` blocks construction and cleanup. Unknown
  getter/layout/state also blocks. This reader has not been exercised in the game
  and does not assert a standalone busted-state detector.
- `VehicleParams` construction calls the SDK's native AddTypeName routine. Factory
  `0x689820` takes that 0x30-byte object by value and returns ISimable. The SDK
  performs the compiler-defined PVehicle adjustment; no manual subtraction or
  SDK-sized PVehicle allocation is used.
- Both registries are captured before/after construction. Returned interfaces must
  be new to the prior snapshot, present in the fresh snapshots, and refer to the
  same native simable/model. All prior numeric registry memberships must remain.
  A failed/partial construction disables further attempts for the session; it does
  not guess a pointer to delete. Such a fault could leave a partial native object,
  which is another reason this adapter is not dispatched yet.
- Once membership is verified, the new vehicle is immediately deactivated and its
  native handle is recorded. That is not proof that construction itself starts
  inactive; hidden candidate evidence is required before calling the factory.

## Verified eviction route

The factory calls `0x6ED260(position, true)` before allocating PVehicle. That function
reads the **physics instance** counter at `0x9377C8`, selects limit 0x40 (64) for
true and 0x34 (52) for false, and can call Kill on a selected distant vehicle when
the counter exceeds its limit. At equality it returns false. A null VehicleCache
does not bypass this separate route.

A MinHook detour preserves the original behavior outside the synchronous constructor
on its gameplay thread. Inside that scope it returns true only for a readable counter
strictly below the chosen limit and never runs the native eviction branch. Other
mods patching an inspected entry or already owning that MinHook target cause setup
to fail closed. This guards the identified route; it is not a claim that all engine
allocation/reentrancy behavior has been proven safe. Exact function bounds are
`0x6ED260..0x6ED380` inclusive; the 256-byte compatibility fingerprint covers its
entry prefix, not the complete routine.

## Racer and retirement contract

Driver conversion can replace the AI, so it is resolved again afterwards. Only the
observed racecar AI interface vtable 0x892640 is accepted. Native SetGoal 0x422480
owns goal replacement; the SDK inline helper is deliberately avoided because it
also clears the old goal. The resulting goal must use Racer vtable 0x892D30.
Preparation leaves the vehicle inactive; it does not make the car move.

Cleanup requires fresh registry membership and matching simable/native handle/model
in the same safe world context. It invokes native Deactivate and `0x6851D0` on the
owned ISimable, never C++ delete/free. Retirement is asynchronous. Removal is confirmed
only after both registries omit the numeric identities in two different completed
gameplay frames, without dereferencing the retired object. Numeric membership and
handle checks reduce stale-pointer risk; they do not prove absence of every ABA
reuse. A changed world context blocks further dereferences and cleanup.

## Validation

- Local C++ capacity tests exhaust both thresholds (0..128), unreadable counters,
  equality and UINT_MAX. These test guard policy only, not native allocation.
- The Win32 ABI target checks VehicleParams/PVehicle prefixes plus AIVehicle goal
  offset 0xB8, pursuit offset 0xBC and ISimable handle offset 0x08. It prints the
  compiler's cooldown offset; its test executes no engine calls.
- Windows build and full repository CI are required before merging this change.
- Road reset, activation, streaming, visual identity, movement, pursuit transitions,
  partial failure behavior and native cleanup remain actual-game tests.
