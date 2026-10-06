# Stakes and Pink Slips

## Goal

Wagers should feel like a negotiation between two street racers, not a generic race-entry fee.

## Supported stake concepts

- Practice / no stake
- Street Rep
- Cash
- Cash + Rep
- Pink Slip
- Car + balancing cash
- Double-or-nothing rematch (future)

## Negotiation flow

During staging:

```text
Rival proposal
-> eligibility evaluation
-> player choices
-> optional counteroffer
-> rival personality decision
-> contract confirmation
-> countdown
```

The contract is frozen before the race starts.

No stake terms may silently change mid-race.

## Last-car rule

A player cannot wager a vehicle if doing so could leave them with zero usable owned cars.

Initial rule:

```text
playerEligibleOwnedCars >= 2
rivalEligibleGarageCars >= 2
```

Both sides follow the same rule.

## Insufficient cash

If the proposed cash stake exceeds the player's balance:

- cash option is disabled
- the reason is shown
- pink slip may be offered only if all pink-slip safety requirements pass
- reputation/practice may be offered based on rival personality
- the rival may refuse and leave

The system must never drive the player's money negative.

## Pink-slip safety gates

Pink Slip becomes selectable only when all conditions are true:

```text
PinkSlipFeatureVerified
AND SupportedExecutable
AND SafeGarageBridge
AND PlayerEligibleCars >= 2
AND RivalEligibleCars >= 2
AND SafeDestinationGarageSlot
AND NoUnsafeTransition
AND TransactionJournalAvailable
```

If any gate fails, the feature is unavailable.

## Street Value

A future Street Value model estimates fair exchange value:

```text
base car value
+ performance investment
+ rare/unique part modifier
+ progression tier
+ optional customization modifier
```

This is not meant to mirror real-world market value.

It exists to prevent absurd wagers such as a stock starter car versus an endgame exotic with no compensation.

## Mixed stakes

If both cars are eligible but values differ:

```text
Player car: 120,000
Rival car:   75,000
Difference:  45,000
```

The weaker side may add cash:

```text
Rival car + 45,000
vs
Player car
```

Only cash the side actually owns may be offered.

## Losing a car

Target behavior:

1. contract resolves player loss
2. transaction enters Prepared
3. exact player car snapshot is captured
4. engine-backed ownership transfer/removal occurs
5. a safe replacement owned car is selected
6. game garage state is verified
7. rival virtual garage receives the won-car snapshot
8. transaction commits

If any destructive step cannot be verified, rollback wins over realism.

## Rival using the won car

A defining feature:

A rival who wins the player's car may later appear driving that exact vehicle configuration, once exact customization serialization is proven safe.

This enables:

- revenge encounter
- recovery race
- increased grudge
- unique dialogue/UI line
- recognizable persistent history

## Garage-full behavior

Do not award a pink-slip car if the destination garage cannot safely receive it.

Initial policy:

- block the pink-slip contract before race start
- explain that no safe garage slot is available

A future "claim queue" is possible only if it can be implemented without creating a second unreliable source of ownership truth.

## Confirmation

Pink-slip contracts require deliberate confirmation.

Recommended interaction:

```text
PINK SLIP
Winner takes the opponent's car.

Hold Accept to confirm.
```

A single accidental button press must never wager a car.
