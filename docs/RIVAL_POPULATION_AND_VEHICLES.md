# Rival Population and Vehicle Selection

## Core answer: cars are not purely random

Free Roam Rivals uses a hybrid population.

There are three distinct layers:

### 1. Authored persistent rivals

Examples: Marcos, Maya, Diego and future named/legendary rivals.

They have a real persistent garage. Their car is **not re-rolled every time
they spawn**.

A rival may change car because:

- they bought/upgraded another vehicle
- they lost a pink slip
- they won the player's vehicle
- their progression advanced
- a scripted legendary/rematch rule changed their active vehicle

If Marcos owns a Supra and the player sees him twice in one session, he should
still be Marcos in that Supra unless something meaningful changed.

### 2. Procedural local racers

These make Rockport feel populated without requiring hundreds of hand-written
characters.

A procedural local is generated from:

- current career ceiling
- Free Roam Rivals Street Rep
- district
- rarity weights
- personality archetype
- deterministic seed

The first vehicle choice is weighted and progression-aware.

Once the player meaningfully interacts with that local (accepted challenge,
race, wager, repeated encounter), the local is promoted to a persistent rival
record. From then on its identity and garage stop re-rolling.

This prevents the player from reloading the game to fish for a rare car.

### 3. Vanilla ambient traffic

Normal traffic remains normal traffic.

Free Roam Rivals does **not** turn every civilian car into a racer and does not
replace the game's traffic population with exotics.

## Vehicle tiers

Initial vanilla catalog:

### Tier 1 — starter/local

- cobaltss
- punto
- gti
- is300
- a3
- clio

### Tier 2 — established street

- rx8
- eclipsegt
- tt
- a4
- mustanggt
- gto
- monaro
- clk500
- cts

### Tier 3 — serious tuned

- supra
- rx7
- imprezawrx
- lancerevo8
- elise
- caymans
- sl500

### Tier 4 — elite

- 997s
- corvette
- db9
- gallardo
- viper
- sl65

### Tier 5 — endgame exotic

- fordgt
- murcielago
- slr
- 911Turbo
- carreragt

### Legendary/special — not normal random traffic

- 911gt2
- camaro
- bmwm3
- bmwm3gtr
- bmwm3gtre46
- corvettec6r

These special cars are opt-in/condition-based and are not part of ordinary
procedural selection.

## Progression-aware selection

The selector consumes the same maximum rival tier produced by the progression
system.

Example:

```text
early career
  -> tiers 1-2

mid career
  -> tiers 1-3

late career
  -> tiers 2-5

career complete / Rockport Legend
  -> full normal catalog
  -> legendary pool only when its own conditions pass
```

Street Rep changes probability and rival interest, but does not allow a new
career profile to be flooded with Carrera GTs.

## District identity

District preference modifies probability rather than hard-locking cars.

Examples:

- Rosewood: more starter/tuner cars
- Camden: more tuner/muscle cars
- Downtown: more European/sports cars
- Highway: more high-performance/exotic cars

A Supra can still appear outside Camden. It is simply more likely in the
districts that fit its profile.

## Determinism and persistence

Vehicle selection uses a deterministic seed.

For a newly generated rival:

```text
profile identity
+ generated rival id
+ progression generation
-> seed
-> weighted car selection
```

After the rival becomes persistent, the selected key is stored.

The selector is not called again for every spawn.

## Visual customization

The physical car should also look like it belongs to the driver.

Planned style archetypes:

- Sleeper
- Clean Street
- Tuner
- Aggressive
- Show Car
- OEM+

NFSPluginSDK exposes engine-side MW05 primitives for:

- RideInfo
- SetStockParts
- SetRandomPaint
- SetRandomParts
- SetRandomPart by upgrade level
- FECustomizationRecord::WriteRideIntoRecord

We should not call fully random visual generation every time a rival appears.
Instead:

1. generate/capture customization once
2. store the resulting customization snapshot in the rival's mod persistence
3. recreate the same visual car on later encounters

That same snapshot model is required for a rival who wins the player's car.

## Performance

Performance is tied to rival tier/skill and progression, not random chaos.

A low-skill Cobalt should not silently receive endgame physics just because an
RNG roll was high.

The planned performance generator chooses a bounded upgrade package from the
rival's progression band and then validates actual corrected performance.

## Add-on cars

The catalog will eventually support custom pvehicle keys.

Runtime policy:

```text
configured vehicle key
-> query pvehicle collection
-> collection exists: eligible
-> missing: skip and log once
```

Therefore an installation with add-on cars can opt them into a tier/pool
without making those cars mandatory dependencies.

The default release remains vanilla-safe.

## Anti-repetition

The selector softly avoids the exact previously selected vehicle when other
eligible candidates exist.

It does not guarantee every visible racer has a unique model. Rockport should
feel plausible, not artificially curated so that no two people own the same
car.

## Pink-slip consequence

When full ownership transfer is safe:

- rival loses wagered car from its virtual garage
- winner gains ownership
- active vehicle is reselected from the remaining garage
- last-car protection applies to both sides
- a rival that won the player's car may later appear driving the exact captured
  customization/performance snapshot

This is why persistent garages are more important than pure random spawns.


## v0.0.7 procedural identity model

The procedural layer now has a pure deterministic generator in
`RivalPopulation.cpp`.

A new local is generated from one seed into a stable bundle:

```text
seed
 -> rival id
 -> display name
 -> vehicle selection
 -> visual archetype
 -> personality
 -> challenge style
 -> starting cash
 -> visual seed
 -> performance seed
```

This matters because a random racer must not become a different person every
time the world reloads.

Before meaningful interaction, a local may remain ephemeral. When the player
accepts a challenge or otherwise causes promotion, the whole generated bundle
is stored as a persistent rival. From that point forward the rival keeps the
same identity until an actual gameplay event changes it.

### Population budget

The city does not fill every road with tuned racers.

Initial budget logic:

```text
early career:
  up to 1 live rival

mid/late career:
  up to 2 live rivals

Rockport Legend:
  usually 2
  up to 3 at high Street Rep
```

These slots are independent of vanilla civilian traffic.

Later the World Director may reserve one slot for a revenge rival, legendary
sighting or district event.

### Personality is generated with the person

A procedural rival receives persistent values for:

- skill
- aggression
- confidence
- risk tolerance
- police fear
- visual archetype
- challenge style

Those values are not re-rolled on every meeting.

A high-aggression rival is more likely to use a Provoker approach. A confident
rival may overtake and slow down. A lower-risk rival may prefer a clean invite
and later be more willing to abandon a race when police pressure becomes high.

### Car ownership rule

The selected car is the rival's first owned vehicle, not a temporary skin.

After promotion the intended model is:

```text
Rival
  -> Garage
       -> active car
       -> optional additional cars
       -> cars won by pink slip
```

Therefore a rival can later change cars for a reason, but never because the
spawn code rolled a new model by accident.
