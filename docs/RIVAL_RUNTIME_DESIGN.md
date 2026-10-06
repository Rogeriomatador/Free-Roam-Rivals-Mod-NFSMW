# Rival Runtime Design

The first real rival must be reliably owned and cleaned up before it is clever.

A valid rival is not merely a car model that appeared. It is a lifecycle-managed engine vehicle whose ownership, AI, visibility and cleanup are known at every frame.

## Runtime identity

A runtime RivalHandle should eventually contain:

~~~
persistent rival ID
runtime generation
IVehicle pointer
validated PVehicle candidate
AI vehicle pointer
runtime state
spawn ownership flag
pending-destroy flag
~~~

Raw engine pointers are never persisted to disk.

## Generation model

Menus, loading and stock races may destroy and rebuild world objects. Each world load gets a generation number. A RivalHandle from an older generation is invalid even if an address happens to remain readable.

## First spawn milestone

The first mutation test should do only this:

1. confirm FreeRoamCandidate for consecutive gameplay ticks
2. confirm the Human IVehicle and validated PVehicle bridge
3. choose a point outside the camera and away from the player
4. construct one conservative Traffic-class car
5. place it on the road
6. verify it entered the live IVehicle registry
7. allow native traffic behavior
8. keep it alive briefly
9. destroy it through the engine
10. verify it leaves the live registry

No challenge, cash, race or persistence mutation in this milestone.

If any step fails, clean up and disable spawning for the session.

## Second milestone — stable roaming rival

After lifecycle proof:
- choose a normal racer vehicle
- keep one active rival
- assign a persistent rival ID
- use road navigation / drive targets
- maintain sensible min/max distance from player
- avoid visible spawn/despawn
- recover when the vehicle becomes invalid

## Encounter adapter

The existing EncounterStateMachine remains engine-independent.

The runtime adapter supplies distance, relative speed, visibility, player/rival validity, pursuit state and district.

The state machine produces:

~~~
Roaming
Interested
ChallengeAvailable
Accepted
Cooldown
~~~

AI behavior reacts to that result.

## Provocation behaviors

Add one at a time:
1. MatchSpeed
2. OvertakeAndSlow
3. PullAheadAndWait
4. SideBySide
5. RevengeHunt

Each behavior requires entry conditions, timeout, road-safety checks, pursuit policy and a fallback to Roaming.

## Race handoff

Accepted does not enter the stock race lifecycle.

~~~
Accepted
-> EncounterDirector
-> optional StagingDirector
-> FreeRoamRaceManager
-> Outrun first
~~~

Outrun is the first mode because it works without an authored route and preserves traffic/police.

## Ownership boundaries

RivalManager owns identity, personality, persistent garage and runtime handle.

SpawnManager owns creation/destruction and safe spawn placement.

RivalAI owns desired driving behavior and engine AI commands.

EncounterDirector owns player/rival interaction state.

RaceManager owns the active duel and result.

EconomyManager owns the frozen wager contract.

A module must not silently mutate another module's state.

## Failure rules

If rival vehicle pointer disappears, invalidate the runtime handle and move the persistent rival to cooldown.

If player enters a vanilla race, remove/suspend Free Roam rival activity before stock race loading completes.

If NIS begins, stop encounter logic and never spawn/present wager UI.

If pursuit begins, an active duel may continue; ordinary roaming reaction is personality-driven.

On world unload, invalidate all runtime handles before any later tick can use them.
