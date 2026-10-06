# Rival Spawn and AI Plan

## Goal

Create one durable free-roam rival without reusing traffic as a fake racer and
without leaking game objects.

## Why spawning is the next hard boundary

Creating a car model is only one part of a rival.

A complete rival needs all of these layers:

```text
PVehicle
  |
  +-- rigid body / physics
  +-- customization
  +-- Racer driver class
  +-- IVehicle interface
  +-- IVehicleAI / racer host
  +-- AIGoalRacer
  +-- WRoadNav
  +-- lifecycle ownership
```

If one layer is missing, symptoms may include:

- car appears but does not drive
- invisible/unsimulated car
- car falls through world
- AI drives straight into geometry
- car cannot despawn cleanly
- stale pointer crash after streaming transition

## Phase S0 — observation

Status: current v0.0.5-dev.

Prove:

- FreeRoamCandidate is stable
- player IVehicle is stable only within a world session
- independent PVehicle player cross-check succeeds
- road network exists in free roam
- Career profile reads correctly
- transitions do not crash

No spawn.

## Phase S1 — construction probe

Experimental build, explicit opt-in.

Only in validated free roam:

1. find a safe road position far from player view
2. construct one `DriverClass::Racer` PVehicle
3. verify it appears in the live vehicle registry
4. resolve its IVehicleAI
5. immediately destroy/kill it through the correct engine lifecycle
6. verify it disappears from both registries

No challenge behavior.

Exit criterion: 50+ create/destroy cycles across world transitions without a
leak or crash.

## Phase S2 — native racer goal

After S1 passes:

1. construct Racer
2. resolve IVehicleAI
3. push/set `AIGoalRacer`
4. initialize road nav
5. set a conservative drive speed
6. allow the game AI to tick naturally

Do not inject steering/gas every frame unless native racer goal proves
insufficient.

## Phase S3 — roaming rival

Add mod-owned state:

```text
Spawned
-> Roaming
-> Interested
-> Provoking
-> ChallengeAvailable
```

Rival chooses a road-relative point ahead rather than directly targeting the
player's exact position.

## Phase S4 — lifecycle robustness

The rival must self-destruct safely if:

- player enters garage
- race starts
- NIS begins
- world unloads
- PVehicle becomes invalid
- road network becomes unavailable
- player is busted/teleported
- mod detects an unsupported state

## Spawn visibility policy

Never create a rival where the player can see the pop-in.

Initial policy:

```text
< 300 m              forbidden
300-850 m            candidate only if occluded/off-screen
> 850 m               allowed only if streaming/road state is valid
> despawn threshold   remove when safe
```

Later, camera/frustum checks should replace pure distance heuristics.

## Vehicle selection

Initial experimental rival should use a vanilla vehicle known to exist in the
current installation.

Do not begin with add-on cars.

After the lifecycle is stable, vehicle choice can be driven by:

- career tier
- Street Rep
- rival garage
- district
- previous history

## AI behavior

Use personality to tune decisions, not physics cheats.

Potential knobs:

- target speed
- aggression
- preferred following distance
- overtake/provoke timing
- mistake probability at the director level
- willingness to use NOS when a verified racer-NOS path exists

No direct speed teleport/rubber-band boost for ordinary rivals.

## Challenge interaction

The planned horn challenge remains desirable, but the current clean-room action
table does not expose a simple named Horn action.

Implementation order:

1. inspect actual binding/runtime input state
2. discover the game's horn path safely
3. keep configurable fallback key
4. never steal the horn from normal gameplay

## First visible milestone

The first build worth calling a gameplay prototype is:

```text
one rival appears naturally
-> drives on native road AI
-> notices player
-> slows/matches speed
-> exposes challenge prompt
-> can return to roaming
-> cleans itself up safely
```

No money and no pink slip are required for this milestone.
