# Pink Slip Engine Research

## What is already known

### Career money

```text
cFrontEndDatabase
-> UserProfile
-> CareerSettings::CurrentCash
```

NFSPluginSDK exposes `CareerSettings::AwardCash`, but direct field mutation is
not automatically equivalent to a fully synchronized persistent transaction.

Read access is enabled. Write access remains blocked.

### Player garage

`FEPlayerCarDB` contains:

```text
CarTable[200]
Customizations[75]
CareerRecords[25]
```

A car record stores:

- car handle
- FE key
- vehicle key
- filter bits
- customization handle
- career handle

The customization table stores installed parts, physics upgrades and tunings.

### Known engine methods

```text
AwardRivalCar               @ 0x5A41E0
GetCarByIndex               @ 0x56ECF0
GetCarRecordByHandle        @ 0x56ECC0
GetCustomizationRecord...   @ 0x56F100
GetCareerRecordByHandle     @ 0x56F120
GetNumCareerCars            @ 0x5810E0
GetNumCars                  @ 0x5812C0
FECustomizationRecord::WriteRideIntoRecord @ 0x56F2F0
```

This strongly supports the feasibility of awarding a rival car and preserving
customization data.

## What is not yet verified

No public method located so far gives us a clearly safe:

```text
RemoveExactCareerCar(handle)
```

That missing operation is why full Pink Slip is still blocked.

## Required transaction

Losing a vehicle must never be implemented as "zero out a CarTable slot".

Required model:

```text
validate
-> snapshot exact FECarRecord
-> snapshot FECustomizationRecord
-> snapshot FECareerRecord
-> snapshot CurrentCar
-> prepare rival-side snapshot
-> engine-backed removal/transfer
-> choose valid fallback CurrentCar
-> verify FEPlayerCarDB
-> request/observe normal save synchronization
-> commit rival state
```

Any failure after snapshot must trigger rollback.

## Last-car rule

Before a loss is even offered:

```text
GetNumCareerCars() >= 2
```

Later this becomes "eligible usable career cars >= 2" to exclude invalid or
non-transferable records.

## Recovery design

When a rival wins the car, Free Roam Rivals stores a serialized *copy* of the
vehicle identity/customization in its own persistence file.

That copy is not considered ownership authority.

The game's FEPlayerCarDB remains authoritative.

The rival snapshot only allows the mod to reconstruct the visual/performance
identity for a future recovery race once the transfer path is verified.

## Development policy

Pink Slip UI may be prototyped before write support.

Actual ownership transfer must stay disabled until:

- safe exact-car removal exists
- autosave interaction is understood
- crash journal exists
- rollback test passes
- only-one-car test passes
- custom bodykit/vinyl/performance test passes
