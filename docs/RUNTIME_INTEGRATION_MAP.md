# Runtime Integration Map

This is the wiring diagram for Free Roam Rivals. It records which game subsystem each feature depends on, what public reverse-engineering source backs it, and whether the project considers it safe to use.

## Confidence tiers

Tier A — verified / preferred: byte-verified retail v1.3 research or a stable engine primitive with a known ABI. Examples: MWSDK live IVehicle registry, IVehicle GetDriverClass, WRoadNetwork singleton, nfsmw-2005-sdk hooks.

Tier B — established but guarded: battle-tested community API or well-understood game call, but not every surrounding lifecycle rule is proven. Examples: PVehicle Construct, AIVehicle goals, ForceAIControl, FEPlayerCarDB AwardRivalCar.

Tier C — research-only: understood enough to investigate, not safe enough to mutate the player's career yet. Examples: deleting/transferring one exact career car and forcing a fully synchronized autosave after a custom ownership transfer.

## Runtime backbone

~~~
speed.exe exact supported v1.3
        |
        +-- VersionGuard
        |     MD5 + size
        |
        +-- FrameCoordinator
        |     +-- D3D9 EndScene       read-only observation
        |     +-- input poller        diagnostic/input later
        |     +-- future GameFrameTick gameplay mutation after ABI validation
        |
        +-- GameBridge
              +-- world state
              +-- race/free-roam state
              +-- live vehicle registry
              +-- player vehicle
              +-- road network
              +-- career/garage read view
~~~

No high-risk subsystem may scatter raw game addresses around the codebase. It must go through GameBridge or a dedicated bridge.

## Free-roam detection

We intentionally do not equate one global value with Free Roam.

The candidate gate is:

~~~
in-world state
AND live Human IVehicle
AND GRaceStatus exists
AND GRaceStatus.PlayMode == Roaming
AND !GRaceStatus.mIsLoading
AND !IsInNIS
AND !IsFadeScreenOn
~~~

A stock race is in-world with a live Human IVehicle and GRaceStatus.PlayMode == Racing.

This distinction is critical: rival spawning must never occur inside a vanilla race unless a future feature explicitly requests it.

## Live vehicles

Preferred source: MWSDK's byte-verified live IVehicle registry.

Entries are IVehicle interface pointers and are classified through the verified GetDriverClass slot:

Human, Traffic, Cop, Racer, None, NIS, Remote.

Rules:
- never cache the player's IVehicle across transitions
- re-resolve every use/gameplay tick
- never reinterpret an IVehicle pointer as PVehicle directly

MWSDK establishes that the registered IVehicle subobject is PVehicle + 0xAC. Free Roam Rivals derives a candidate PVehicle base only after the exact executable passes, the pointer came from the live registry, memory is readable, and the candidate vtable matches the expected PVehicle family. This cross-SDK bridge stays diagnostic until our target-machine logs prove it.

## Rival spawning

Public mod implementations demonstrate that the engine can construct arbitrary PVehicle objects with a driver class, vehicle key, position, direction, customizations and flags.

Planned creation path:

~~~
FreeRoamGate
-> SpawnPlanner
-> safe off-camera location
-> PVehicle::Construct
-> SetVehicleOnGround
-> verify live-registry membership
-> assign AI behavior
-> RivalHandle owns lifecycle
~~~

The first experimental spawn should use Traffic rather than Racer. Traffic is designed to navigate Free Roam. Racer AI is mapped, but may assume stock race route/GRaceStatus data.

## Rival AI

The engine AI has two layers: AIGoal is strategic state and AIAction is tactical behavior.

Mapped goals include AIGoalNone, AIGoalTraffic, AIGoalRacer, AIGoalPursuit, AIGoalPatrol, AIGoalRam, AIGoalPit and AIGoalStopShort.

Useful AIVehicle operations exposed by NFSPluginSDK include SetGoal, SetDriveSpeed, SetDriveTarget, ResetDriveToNav, ResetVehicleToRoadNav, ResetVehicleToRoadPos, GetCurrentRoad and GetFutureRoad.

Free Roam Rivals must not fake normal behavior through constant teleporting. Teleporting is reserved for hidden recovery/cinematic correction.

## Roads and route systems

There are two different concepts.

Traffic paths are geometry for vehicle following. They are useful for natural roaming, spawn alignment and lane-aware placement.

CARP is a node/segment graph for routing and junction connectivity. It is useful for destination races, route distance, staging search, checkpoint generation and GPS.

MWSDK decodes both and exposes the live WRoadNetwork singleton.

Long-term staging search:

~~~
player/rival position
-> nearest road graph
-> candidate forward segments
-> reject junctions and bad curvature
-> score width/straightness/visibility
-> reserve two staging poses
~~~

## Staging and camera

Staging phases are Search -> Reserve -> Approach -> Align -> CameraIntro -> StakeUI -> Countdown -> Release.

Player control may later use the engine's ForceAIControl/ClearAIControl path, but only from a validated gameplay-thread callback.

The render callback is observation/HUD only. It must never move vehicles or commit gameplay state.

Cinematics are realtime rather than dynamic VP6. Every exit path must restore input, camera state, camera updating and staging state.

## Challenge input

The current clean-room action table does not expose an obvious HORN action. We will not guess an input-mirror offset.

Research order: observe input poller, inspect XtendedInput interaction, locate the horn action/audio path, retain a configurable fallback key, then make horn default only after proof.

## HUD

Initial HUD should be an independent D3D9 overlay for challenge icon, rival card, wager UI, Outrun meter and result. This avoids replacing FRONTEND/HUD archives and minimizes conflicts with texture packs/translations.

## Career/economy

NFSPluginSDK exposes CurrentCash, CurrentCar, current career bin, career-completed flag, FEPlayerCarDB, career-car count and car/customization/career records.

Cash writes are technically simple, but remain disabled until transaction and save synchronization are validated.

## Pink slips

Player wins rival car: FEPlayerCarDB AwardRivalCar is a known engine path, but the resulting record and autosave behavior still need validation.

Player loses exact car: this remains the hardest mutation. We need exact owned-record identification, complete customization/career snapshot, an engine-backed remove/transfer operation, safe fallback current car, ownership verification, save synchronization and rollback.

Raw save-file surgery remains prohibited.

## Police

Pursuit is parallel to race mode. A Free Roam Rivals duel should normally continue while police are involved; rival personality may choose to abort. We should not globally disable cops for a duel.

## Threading

Render callback: read-only observation and future HUD.

Input callback: input observation only initially.

Gameplay/main tick: only place allowed to spawn, kill, reposition, change AI goals or commit race state.

Background threads: persistence serialization, file IO and logging our own atomics only; never dereference live engine objects.

## Hard mutation gates

All default OFF:

~~~
ExperimentalSpawnEnabled=0
ExperimentalAIControlEnabled=0
ExperimentalEconomyWritesEnabled=0
ExperimentalGarageWritesEnabled=0
~~~

A future build enables one capability at a time after the preceding read-only validation passes.
