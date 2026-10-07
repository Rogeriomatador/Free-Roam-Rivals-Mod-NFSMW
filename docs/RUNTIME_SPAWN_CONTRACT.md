# Runtime Rival Spawn Contract

This document defines the exact safety contract that must be satisfied before
Free Roam Rivals is allowed to create a live Racer in Rockport.

## Engine chain

Public MW05 research currently gives us this chain:

```text
Vehicle selection
  -> pvehicle key exists
  -> FECustomizationRecord prepared
  -> VehicleParams
       DriverClass::Racer
       vehicle key
       direction
       position
       customization
       SnapToGround | CalcPerformance
       GRaceStatus as IVehicleCache
  -> PVehicle::Construct
       game constructor @ 0x689820
  -> PVehicle
       mAI -> IVehicleAI
  -> Racer host / goal
       CreateAIVehicleRacerInstance @ 0x43EF70
       CreateAIGoalRacerInstance    @ 0x43D388
       SetAIRacerGoal / PushAIGoalByHash
  -> WRoadNav
  -> native AI tick
```

The important rule is that creating a PVehicle alone is not enough. A complete
rival needs a valid AI host, goal and road navigation state.

## Pre-spawn gates

Every gate must pass:

```text
supported executable
AND FreeRoamCandidate
AND GRaceStatus exists
AND GRaceStatus::PlayMode == Roaming
AND !GRaceStatus::mIsLoading
AND !IsInNIS
AND !fade
AND valid Human IVehicle
AND independent player PVehicle cross-check
AND road network exists
AND selected pvehicle key exists
AND spawn point passes road/visibility validation
AND current live rival count < population budget
```

Any failed gate means: do nothing.

## Spawn point contract

Initial spawn candidates must be road-relative, not arbitrary XYZ points.

Candidate must provide:

- valid road segment
- lane/forward vector
- ground-valid position
- enough longitudinal space
- no immediate overlap with player
- no immediate overlap with another live vehicle
- outside no-spawn radius
- preferably occluded/off-screen

Initial distance band remains conservative:

```text
< 300 m      forbidden
300-850 m    candidate when hidden/off-screen
> 850 m      only when road + streaming state are known valid
```

## AI bootstrap

The first experimental rival should use the engine's Racer driver path.

Do not manually inject steering/gas every frame as the default design.

Preferred order:

1. Construct Racer PVehicle.
2. Verify PVehicle registry contains it.
3. Verify live IVehicle registry contains DriverClass::Racer.
4. Resolve IVehicleAI.
5. Initialize/reset road navigation.
6. Set conservative drive speed.
7. Install/verify native Racer goal.
8. Observe several seconds of native AI movement.
9. Only then expose the object to EncounterDirector.

## Roaming behavior

A free-roam rival should not require a stock race route.

For the first prototype, roaming may use road-relative drive targets:

```text
current road
 -> future road
 -> point ahead
 -> SetDriveTarget
 -> ResetDriveToNav when required
```

The final system should prefer native goal/path behavior wherever possible.

## Interest / provoke integration

Once stable:

```text
Roaming
 -> player enters InterestRadius
 -> personality evaluates interest
 -> rival picks an approach target
 -> MatchSpeed / OvertakeAndSlow / PullAheadAndWait
 -> ChallengeAvailable
```

The rival must never continuously target the player's exact transform because
that tends to produce collisions and unnatural chasing.

## Cleanup contract

Cleanup is as important as creation.

The rival must be removed/invalidated safely when:

- world leaves Free Roam
- stock race begins
- NIS begins
- garage/safehouse transition begins
- player vehicle becomes invalid
- road network disappears
- rival is destroyed
- plugin detects a registry mismatch
- world unload starts

Preferred cleanup research order:

```text
IVehicleAI::UnSpawn()
 -> verify registries
 -> PVehicle::Kill() only through verified lifecycle
 -> clear all mod-owned pointers/handles
```

Never call Kill blindly while engine subsystems still own the object.

## Pointer policy

No live engine pointer survives a world transition.

The mod stores stable mod-side rival IDs. Runtime PVehicle/IVehicle/IVehicleAI
pointers are session handles only and must be reacquired/validated.

## Experimental enablement

The experimental spawn feature remains behind:

```ini
[Experimental]
ExperimentalSpawnEnabled=0
ExperimentalAIControlEnabled=0
```

Even after code exists, default public builds keep those switches off until
create/drive/cleanup tests pass.

## Promotion criteria

`rivalSpawnExperimentVerified` may become true only after:

- 50+ create/drive/cleanup cycles
- multiple Free Roam -> menu -> Free Roam transitions
- garage transition
- stock race transition
- pursuit active/inactive cases
- rival destruction case
- no stale PVehicle/IVehicle entries
- no leaked Racer entries
- no crash on game exit
- compatibility test with the current ASI stack
