# Save and Garage Safety

Pink-slip racing is the highest-risk feature in the project because a bug can destroy progression.

This document is binding engineering policy.

## Absolute rules

1. Never remove a player's last usable car.
2. Never mutate career garage state on an unsupported executable.
3. Never raw-patch the save file as the primary implementation.
4. Never mark a transaction successful before verifying the resulting garage/economy state.
5. Never delete the pre-transaction snapshot until the transaction is committed and verified.
6. A failure must prefer duplicating/retaining a car over destroying the only authoritative copy.

## Eligibility

Player may offer a pink slip only if:

```text
owned_eligible_cars >= 2
AND current_car is transferable
AND not in unsafe game transition
AND garage bridge verified
```

Rival may offer a pink slip only if its virtual garage has at least two usable vehicles unless the design explicitly treats the rival as retiring after the race.

Initial policy: require 2+ for both.

## Transaction model

```text
T0 Validate
T1 Snapshot player garage/economy metadata
T2 Snapshot rival virtual garage
T3 Prepare destination record
T4 Apply engine-backed transfer
T5 Verify player garage
T6 Verify rival state
T7 Commit mod persistence
T8 Release snapshots after delayed verification
```

If T4–T7 fails:

```text
Rollback
-> verify rollback
-> disable further pink-slip transactions for session if uncertain
-> log transaction ID + reason
```

## Crash recovery

Before a high-risk transaction, write a small journal:

```json
{
  "transaction": "uuid-or-monotonic-id",
  "state": "prepared",
  "kind": "pink_slip",
  "player_car_snapshot": "...",
  "rival_id": "marcos",
  "timestamp": "..."
}
```

On plugin startup:

- if no journal: normal boot
- if committed: archive/remove journal
- if prepared/applying: do not perform another garage transaction; enter recovery mode

## Save ownership

The mod's own JSON is not authoritative for whether the game owns a car.

The game garage is authoritative.

The mod's data describes rivals and relationships.

## Testing gate for full pink slips

Before v0.7 release, run at minimum:

- win car with 2 player cars
- lose car with 2 player cars
- attempt lose with exactly 1 car -> must block
- cancel at wager screen
- abort during staging
- pursuit begins during race
- player busted after race
- quit immediately after result
- crash simulation before commit
- crash simulation after game transfer but before mod persistence
- multiple sequential pink slips
- custom bodykit/vinyl/performance car
- bonus/special car eligibility
- full garage / capacity edge case
- invalid rival vehicle
- unsupported executable

No destructive path ships without rollback coverage.
