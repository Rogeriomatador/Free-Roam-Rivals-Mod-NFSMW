# Architecture

## High-level modules

```text
PluginBootstrap
  |
  +-- VersionGuard
  +-- Config
  +-- Log
  +-- GameBridge
        |
        +-- GameState
        +-- VehicleBridge
        +-- InputBridge
        +-- WorldBridge
        +-- CameraBridge
        +-- HudBridge
        +-- GarageBridge [later]
  |
  +-- RivalManager
        +-- Rival
        +-- RivalAI
        +-- RivalPersistence
  |
  +-- EncounterDirector
        +-- InterestEvaluator
        +-- ChallengeFlow
        +-- StagingDirector
  |
  +-- RaceManager
        +-- OutrunRace
        +-- DestinationRace [later]
  |
  +-- EconomyManager
        +-- StakeManager
        +-- PinkSlipManager [later]
  |
  +-- SafetyManager
        +-- Transaction
        +-- Snapshot
        +-- Rollback
```

## Core rule: game access is isolated

Gameplay systems must not scatter raw game addresses around the codebase.

Only `GameBridge` may talk directly to SDK/runtime details.

Example:

```cpp
// Good
auto player = game.vehicle().player();

// Bad
auto player = *reinterpret_cast<void**>(0xDEADBEEF);
```

This makes executable support and future SDK changes manageable.

## Rival state machine

```text
Dormant
Candidate
Spawned
Roaming
Interested
Provoking
ChallengeAvailable
Accepted
Staging
Negotiating
Countdown
Racing
PostRace
Cooldown
DespawnPending
```

Each transition must have:

- entry action
- exit action
- timeout
- validation guard
- recovery path

A rival that loses its vehicle pointer must recover to a safe state, never leave the plugin holding a dangling pointer.

## EncounterDirector

Responsibilities:

- choose whether a rival notices the player
- control approach/provocation behavior
- expose challenge availability
- validate whether staging is safe
- hand off to race flow

It does **not** own money, save data or race scoring.

## StagingDirector

Stages:

```text
Search
Reserve
Approach
Align
CameraIntro
StakeUI
Ready
Release
```

Required fallback:

If either vehicle fails to align before timeout:

1. switch to a camera angle that hides correction
2. validate destination transform
3. align vehicle
4. continue

If transform validation fails, abort the encounter cleanly and return control.

## RaceManager

RaceManager owns one active race at a time.

Initial interface:

```cpp
struct RaceResult {
    enum class Winner { None, Player, Rival, Aborted };
    Winner winner;
};

class IRaceMode {
public:
    virtual ~IRaceMode() = default;
    virtual void Begin() = 0;
    virtual void Tick(float dt) = 0;
    virtual RaceResult Result() const = 0;
    virtual void Abort() = 0;
};
```

## Outrun

Data:

- player position
- rival position
- current separation
- leading side
- lead hold timer
- max allowed world distance / invalidation rules

The displayed progress should communicate both who is ahead and how close the win condition is.

## Persistence

Do not use the game's save file for rival metadata.

Proposed path:

```text
scripts/FreeRoamRivals/
  FreeRoamRivals.ini
  Rivals.ini
  Saves/
    <profile-key>.json
```

Profile key must avoid storing personally identifying information.

Persistent data:

- rival identity
- rival cash
- garage
- relationship
- W/L history
- cooldowns
- lost/won car snapshots when that system becomes safe

## Transactions

Any operation that changes career economy or garage state must become a transaction:

```text
Validate
Snapshot
Prepare
Apply
Verify
Commit
```

On any failure after Snapshot:

```text
Rollback
VerifyRollback
DisableHighRiskFeatureIfNeeded
Log
```

## Threading

Default rule: gameplay state mutates only from the game/main thread.

Background work, if ever used, may parse config or write copied state but must not dereference live engine objects.

## Logging

File:

```text
scripts/FreeRoamRivals/FreeRoamRivals.log
```

Levels:

- INFO
- WARN
- ERROR
- DEBUG (configurable)

High-risk operations must log transaction IDs.

## Fail-closed behavior

If the executable is unsupported:

- plugin may log that it loaded
- no gameplay hooks are installed
- no save/economy operation is permitted
- user receives a clear diagnostic
