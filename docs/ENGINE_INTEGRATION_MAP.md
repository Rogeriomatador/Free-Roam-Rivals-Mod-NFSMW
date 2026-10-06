# Engine Integration Map

This document records what Free Roam Rivals currently trusts, what it only
observes, and what is still blocked behind a safety gate.

The project intentionally uses three independent public research/code bases:

- **nfsmw-2005-sdk** — hooking, input polling, D3D9 callback, AOB helpers.
- **MWSDK** — verified runtime registry, world state, road-network and live
  IVehicle access.
- **NFSPluginSDK** — typed MW05 gameplay structures such as GRaceStatus,
  career/profile data, PVehicle, AI interfaces and vehicle construction.

Dependencies are pinned to commits in CMake. Release builds do not follow a
moving main/master branch.

## 1. Executable identity

Current supported target:

```text
speed.exe size: 6,029,312 bytes
MD5: C0516B485065FABDD69579816B5DF763
```

Everything that can mutate game state is fail-closed on an unsupported build.

## 2. World state

### Source A — MWSDK

```text
GameStateMode
  -> game_flow_state()
  -> in_world()
```

MWSDK documents state 6 as the in-world/Racing GameFlow state. This is not
enough by itself to distinguish free roam from a stock race.

### Source B — GRaceStatus

```text
g_pGRaceStatus @ 0x91E000
  -> mIsLoading
  -> mPlayMode
       Roaming
       Racing
```

Free Roam classification therefore requires both layers:

```text
in_world
AND GRaceStatus exists
AND mPlayMode == Roaming
AND !mIsLoading
AND !IsInNIS
AND !IsFadeScreenOn
AND live player vehicle exists
```

This prevents the mod from treating an ordinary career race as a roaming
encounter.

## 3. Live vehicles

MWSDK's verified live list contains **IVehicle interface pointers**.

```text
PVehicle_mVehicleListData
PVehicle_mVehicleListCount
  -> IVehicle*
  -> GetDriverClass()
```

Driver classes:

```text
Human
Traffic
Cop
Racer
None
NIS
Remote
```

The unique Human IVehicle is used as the primary player identity.

### Important safety correction

An IVehicle list element is **not** a PVehicle base pointer.

Do not do this:

```text
PVehicle = IVehicle - guessed_offset
```

MWSDK explicitly warns against it.

Free Roam Rivals resolves PVehicle independently through NFSPluginSDK's
validated PVehicle registry and only uses the result as a cross-check.

## 4. PVehicle

NFSPluginSDK exposes:

```text
PVehicle::g_mInstances @ 0x9352B0
PVehicleEx::ValidatePVehicle
PVehicleEx::GetPlayerInstance
PVehicle::Construct(VehicleParams)
```

The read-only runtime probe currently uses this layer only to independently
confirm a valid player PVehicle.

No PVehicle is cached across gameplay transitions.

## 5. Racer creation research

The physical vehicle construction path is known:

```text
VehicleParams
  DriverClass::Racer
  vehicle key
  direction
  position
  customization
  flags
  IVehicleCache / GRaceStatus

PVehicle::Construct(...)
  -> game constructor @ 0x689820
```

This is **not yet considered a complete rival spawn**.

A durable rival additionally needs:

```text
physical PVehicle
+ Racer-class AI host
+ AIGoalRacer
+ road navigation
+ lifecycle ownership
+ clean destruction
+ streaming safety
```

Until all of those are proven together, `rivalSpawnExperimentVerified`
remains false.

## 6. Racer AI

Public reverse engineering maps:

```text
AIVehicle::SetGoal / PushAIGoalByHash @ 0x422480
SetAIRacerGoal                  @ 0x423010/0x42305f research naming
CreateAIGoalRacerInstance       @ 0x43D330/0x43D388
CreateAIVehicleRacerInstance    @ 0x43EF70
AIGoalRacer vtable              @ 0x892720
AIVehicleRacer vtable           @ 0x892AD0
```

NFSPluginSDK also exposes AI operations such as:

```text
SetDriveSpeed
SetDriveTarget
ResetDriveToNav
ResetVehicleToRoadNav
ResetVehicleToRoadPos
SetGoal
SetAiControl
```

The mod will prefer engine-native racer goals over manually writing steering,
gas and brake each frame.

## 7. Road navigation

MWSDK exposes the live WRoadNetwork singleton.

NFSPluginSDK exposes WRoadNav fields including:

- current position
- forward vector
- segment/lane
- left/right road positions
- road spline
- path goal
- validity
- navigation/path/lane types

This is the basis for:

- spawn points hidden from the player
- safe staging areas
- side-by-side starts
- destination races
- road-aware despawn
- avoiding blind teleports into walls

## 8. GPS

Known engine paths include:

```text
GPS::Get / singleton @ 0x90D8E4
Game::EngageGPS      @ 0x42C830
Game::DisengageGPS   @ 0x41ACE0
```

Destination-race navigation can therefore use the real GPS after the route
target is verified.

## 9. Camera / staging

The final design uses a real-time cinematic, not generated VP6 video.

The camera layer will only be enabled after:

- current camera ownership is identified
- camera restoration is guaranteed on every abort
- XtendedInput / Widescreen / X360 Stuff coexistence is tested

Staging must always have a hidden-alignment fallback.

## 10. Career/profile reads

Typed path:

```text
cFrontEndDatabase
  -> UserProfile
      -> CareerSettings
          CurrentCash
          CurrentCar
      -> FEPlayerCarDB
          GetNumCareerCars()
      -> mCareerModeHasBeenCompletedAtLeastOnce
```

These values are currently **read-only**.

They are enough to support:

- last-car protection checks
- career-complete Rockport Legend detection
- cash eligibility preview
- current-car identity research

They do not authorize writes.

## 11. Pink-slip research

Known safe-looking primitives include:

```text
FEPlayerCarDB::AwardRivalCar @ 0x5A41E0
GetCarByIndex
GetCarRecordByHandle
GetCustomizationRecordByHandle
GetNumCareerCars
```

Career records contain handles into separate customization and career-record
tables.

What is still missing is the destructive half:

- remove the exact player-owned car safely
- select a replacement current car
- synchronize autosave/save integrity
- rollback after a crash halfway through the transaction

Therefore:

```text
garageWriteVerified = false
economyWriteVerified = false
```

## 12. Hooking strategy

Current development build uses observation hooks only.

- D3D9 EndScene: primary read-only sampling signal.
- input poller: independent health signal.

The game has a verified main frame entry at `0x663D30`, but Free Roam Rivals
does not need to hook it yet. Fewer hooks means fewer compatibility surfaces.

## 13. Capability promotion rule

A feature moves from research to enabled only after:

```text
known address/layout
+ correct ABI
+ target executable validation
+ runtime test
+ cleanup path
+ compatibility test
+ failure/rollback test when state is persistent
```

Finding an address in an SDK is not enough by itself.
