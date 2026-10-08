# Community Implementation Audit

This file records public MW05 mods that already exercise engine paths relevant
to Free Roam Rivals. It is used as supporting evidence, not copied code.

## NFS Chat Chaos Mod

Repository:
https://github.com/berkayylmao/NFS-Chat-Chaos-Mod

The MW05 effects provide practical proof that direct runtime vehicle
construction and cleanup can work in a real mod.

### Physical spawn

Observed pattern:

```text
player PVehicle
-> GetPosition()
-> GetRigidBody()->GetForwardVector()
-> PVehicle::Construct(VehicleParams(...))
-> SetVehicleOnGround(...)
-> use vehicle
-> PVehicle::Kill()
```

Examples include traffic trucks and cop cars.

This upgrades the following question from "purely theoretical" to
"community-proven primitive":

```text
Can MW05 construct and later kill an extra PVehicle at runtime?
YES, for tested DriverClass values such as Traffic/Cop/None/Remote.
```

It does **not** yet prove our exact `DriverClass::Racer` lifecycle.

### Cop AI attachment

One public effect constructs a Cop-class PVehicle, resolves its AI object and
starts pursuit behavior against the player.

This is important evidence that:

```text
PVehicle::Construct
-> GetAIVehiclePtr()
-> cast concrete AI
-> initialize native AI behavior
```

is a viable runtime architecture.

### Cleanup

The same code validates spawned PVehicle pointers before calling `Kill()`.

That strongly supports `Kill()` as the first cleanup primitive to test in our
S1 spawn probe.

## NFSPluginSDK

Repository:
https://github.com/berkayylmao/NFSPluginSDK

Relevant MW05 primitives:

```text
PVehicle::Construct
PVehicleEx::ValidatePVehicle
PVehicleEx::GetPlayerInstance
PVehicle::Kill
PVehicle::SetVehicleOnGround
PVehicle::GetAIVehiclePtr

AIVehicle::SetGoal
AIVehicle::SetDriveSpeed
AIVehicle::SetDriveTarget
AIVehicle::ResetDriveToNav
AIVehicle::ResetVehicleToRoadNav
```

Racer structures include `AIVehicleRacecar` / `IRacer` with
`PrepareForRace`, `StartRace`, and `QuitRace`.

## s-b-repo reverse engineering

The RE project independently maps the native racer stack:

```text
AIVehicleRacecar constructor @ 0x43D330 (goal factory label rejected by uploaded executable)
CreateAIVehicleRacerInstance @ 0x43EF70
AIVehicle::SetGoal           @ 0x422480
SetAIRacerGoal               @ 0x423010 region
AIGoalRacer vtable           @ 0x892720
AIVehicleRacer vtable        @ 0x892AD0
```

This agrees conceptually with NFSPluginSDK's public AI interfaces.

## Practical conclusion

We have enough evidence to design an experimental S1 spawn build.

What is still not proven on the project owner's heavily modded installation:

1. constructing `DriverClass::Racer` while GRaceStatus is in Roaming mode
2. which concrete AI object is attached by that construction path
3. whether `AIGoalRacer` is sufficient outside a stock race
4. whether racer rubber-band logic assumes stock race data
5. correct road-nav initialization for a free-roam rival
6. cleanup across garage/race/world transitions

Therefore mainline remains read-only until v0.0.5 runtime classification is
confirmed in-game. The next mutation build should perform only one controlled
spawn and one controlled kill before adding challenge logic.
