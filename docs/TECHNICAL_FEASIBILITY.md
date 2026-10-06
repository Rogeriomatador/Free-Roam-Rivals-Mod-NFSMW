# Technical Feasibility

This document separates **known available primitives** from features that still require reverse engineering.

## Runtime foundation — feasible now

Public clean-room SDK/research projects expose enough groundwork to begin a native ASI:

- 32-bit native plugin loading
- inline/vtable hooks
- signature scanning
- D3D9 per-frame hooks for custom HUD
- input access / polling hooks
- hashed game event access
- verified game globals and function addresses
- vehicle/pursuit layouts and attribute access
- world streamer / AI path graph research
- CARP/road-network data research

Primary references:

- https://github.com/s-b-repo/nfsmw-2005-sdk
- https://github.com/s-b-repo/nfsmw-2005-re
- https://github.com/TsyVM/MWSDK
- https://github.com/TsyVM/MWEncyclopedia

## Feature matrix

| Feature | Status | Notes |
|---|---|---|
| Native ASI plugin | Green | Standard and well understood |
| Detect game version / supported exe | Green | Hash/signature guard should fail closed |
| Per-frame update | Green | Multiple hook points are known |
| Custom HUD overlay | Green | D3D9 hooks are documented |
| Read player/racer vehicle state | Green/Yellow | Layouts exist; exact integration chosen during implementation |
| Detect horn/input | Green/Yellow | Input hooks are available; map exact action safely |
| Rival state machine | Green | Entirely mod-owned |
| Rival persistent stats | Green | Store in separate mod file |
| Outrun logic | Green | Mod-owned distance/progress logic |
| Cash wagers | Yellow | Reading/writing career cash must use verified path |
| Road-aware staging search | Yellow | Road network exists; selection heuristics are ours |
| AI lead-to-staging | Yellow | Requires reliable AI goal/action control |
| Cinematic camera | Yellow | Camera systems are RE'd but need safe runtime control |
| Spawn arbitrary racer | Yellow | Vehicle/AI creation path needs focused implementation |
| Give rival car to player | Yellow | Engine-side rival award behavior exists in game; exact callable path must be validated in our target exe |
| Remove exact player-owned car | Orange | Do not implement by raw save surgery |
| Preserve exact customization in transfer | Orange | Requires a verified FE garage/customization bridge |
| Rival later drives lost exact car | Orange | Depends on safe capture/restore of vehicle customization |
| Dynamic pre-rendered FMV | Red / unnecessary | Use real-time camera instead |

## Why a custom race manager

The first race modes should not depend on fully entering the game's stock race lifecycle.

A mod-owned race state lets us preserve:

- free roam
- traffic
- police
- streaming
- current world position

Initial Outrun state only needs:

```text
player vehicle
rival vehicle
race start state
separation/progress
timeout
winner
```

Later route races can add road-graph progress.

## Real-time cinematics

The target is not a generated VP6 movie.

The cinematic is a controlled live scene:

- real current cars
- current paint/customization
- current location
- current lighting
- camera splines/poses
- temporary input suppression

This is both more flexible and more compatible with modded cars.

## Version strategy

Hardcoded addresses without validation are forbidden.

The plugin must:

1. identify supported executable
2. verify critical signatures before installing hooks
3. disable only the affected feature when possible
4. write a useful log
5. never continue into save mutation when verification fails

## Compatibility philosophy

Prefer:

- ASI-only logic
- separate JSON/INI state
- runtime hooks
- feature flags

Avoid:

- replacing GLOBAL archives
- replacing FRONTEND archives
- overwriting language files
- patching user saves directly

This minimizes conflicts with X360 Stuff, Widescreen Fix, HD Reflections, translations and other modpacks.
