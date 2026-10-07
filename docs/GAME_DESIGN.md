# Game Design

## Rival lifecycle

A rival is a persistent entity with a lightweight off-screen simulation and a live in-world representation when active.

```text
Dormant
-> Candidate
-> Spawned / Live
-> Roaming
-> Interested
-> Provoking
-> ChallengeAvailable
-> Staging
-> Negotiating
-> Racing
-> PostRace
-> Roaming / Cooldown / Despawn
```

## Detection and challenge behavior

A rival may become interested based on:

- distance to player
- relative speed
- player Street Rep
- vehicle tier / estimated street value
- previous history
- district
- pursuit state
- rival personality

Possible approach behaviors:

- overtake and slow down
- match speed beside player
- briefly flash lights/horn if an engine-safe signal is available
- pull ahead and wait
- ignore player
- flee from police
- actively seek revenge

### Challenge input

Primary concept: **horn**.

The player should not need to open a menu while driving.

A configurable fallback key must exist for installations where horn detection conflicts with input mods.

## Staging sequence

After mutual acceptance:

1. Find a suitable road segment ahead.
2. Reserve two staging poses.
3. Rival leads or both cars move toward it.
4. Player input is temporarily limited.
5. Vehicles align.
6. Cinematic camera sequence begins.
7. Rival card + wager is shown.
8. Player accepts/declines.
9. Countdown.
10. Control returns on GO.

### Staging fallback

AI parking will not be trusted blindly.

If a car fails to reach its staging pose within a timeout, the camera hides the correction and the car is safely aligned to the target transform.

The player should perceive this as editing/camera direction, not a teleport bug.

## Wager system

### Stake types

- No stake / practice
- Reputation
- Cash
- Cash + reputation
- Pink slip
- Balanced mixed stake (car + cash difference)

### Last-car protection

Pink Slip is unavailable when:

```text
player_owned_drivable_cars <= 1
```

This rule is non-negotiable.

A rival also cannot wager its last usable car.

### Insufficient cash

If a rival requests more money than the player has:

- Cash is disabled.
- Pink Slip may be offered if the player owns 2+ eligible cars.
- Reputation-only may be offered according to rival personality.
- The rival may refuse to race.

The UI should explain *why* an option is unavailable.

### Street Value

A car's wager value should eventually consider:

```text
base vehicle value
+ performance investment
+ rarity / unique parts
+ progression tier
+ optional customization modifier
```

If values are badly mismatched, a mixed stake can compensate:

```text
Player: Carrera GT      value 120,000
Rival:  RX-7            value  75,000
Rival adds cash                +45,000
```

Exact formulas are intentionally deferred until reliable vehicle/economy data is exposed.

## Pink slip ownership

### Player wins rival car

Target behavior:

- rival loses the wagered vehicle from its persistent virtual garage
- player receives a career-owned copy through a verified engine path
- rival can continue only if another valid vehicle exists

### Player loses car

Target behavior:

- exact player vehicle record is transferred/removed safely
- rival gains a serialized representation of that exact vehicle
- player is switched to another owned car safely
- the lost car can later appear on the rival in free roam
- a revenge/pink-slip race can recover it

This feature remains blocked until the save/garage bridge is verified.

## Rival memory

Per rival:

```ini
WinsAgainstPlayer=0
LossesAgainstPlayer=0
Respect=0.0
Grudge=0.0
Fear=0.0
Cash=25000
CurrentCar=RX7
```

Events affect memory:

- clean loss -> respect up
- player repeatedly rams rival -> grudge up
- huge performance gap -> fear up
- rival wins player's car -> rivalry spike
- player recovers a lost car -> rivalry spike

## Race types

### Outrun — first implementation

No fixed route.

Win by opening a configurable distance lead and holding it.

Advantages:

- naturally fits free roam
- requires minimal route infrastructure
- supports shortcuts
- police can remain active
- ideal for v0.2

### Destination

A world destination is selected. First to enter the finish radius wins.

### Sprint

Generated or authored checkpoints.

### Rolling Race

Race starts while both vehicles are already moving.

### High Stakes

A presentation layer over another race type with larger wagers.

## Police

Default policy:

**A pursuit does not automatically cancel a race.**

Potential rival reactions:

- coward: aborts
- pragmatic: finishes if close, otherwise aborts
- fearless: continues
- chaotic: attempts to route police toward player

If the race ends while pursuit is active, pursuit continues.

## Rival vs rival

Later versions may simulate races that do not involve the player.

The player can encounter an active duel and watch it.

Long-term option: allow the player to challenge the winner.

## Street Meets

Occasional clusters of racers in suitable parking areas.

No giant menu is required: approach a vehicle and interact with that driver.

## Legendary rivals

Rare condition-based racers may require combinations of:

- Street Rep threshold
- career progression
- district
- time/visual condition when available
- heat/pursuit history
- wins against specific rivals

Legendary rivals should be discoverable through play, not only UI lists.


## Underground Blacklist — post-career ranked ladder

Completing the vanilla Blacklist does not end Free Roam Rivals.

It unlocks a separate mod-owned ladder called **Underground Blacklist**.

This ladder is not implemented by rewriting the original game's 15 rivals. It
has independent progression and uses persistent Free Roam Rivals opponents.

Flow for each rank:

```text
future rank locked
-> current rival becomes rumored
-> meet Street Rep / qualifier requirements
-> rival becomes eligible to appear in Rockport
-> player must physically find the rival
-> identity is revealed on first sighting
-> later encounter + horn challenge
-> staging / wager / showdown
-> defeat persists
-> next rank unlocks
```

The menu never teleports the player directly into the ranked race.

The current ranked rival is still a real world participant and can use the same
garage, personality, police behavior, wager and history systems as ordinary
persistent rivals.

The screen can initially show silhouettes for undiscovered opponents. Later
photos, voice lines and themes attach to stable rival asset keys without
changing progression data.

See `UNDERGROUND_BLACKLIST.md`.
