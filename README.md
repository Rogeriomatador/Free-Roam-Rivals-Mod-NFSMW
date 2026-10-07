# NFSMW: Free Roam Rivals

**Free Roam Rivals** is a native mod for **Need for Speed: Most Wanted (2005) PC v1.3** focused on making Rockport feel alive outside scripted races.

The goal is not to place static race markers on the map. Rivals should exist as persistent street racers: they drive around Rockport, notice the player, provoke or ignore them according to personality, challenge them naturally, race without loading screens, remember wins and losses, and eventually support high-stakes wagers including pink-slip car races.

## Vision

A normal free-roam session should be able to produce a story like this:

1. A tuned Supra passes the player in Camden.
2. The driver slows down and waits.
3. The player approaches and honks.
4. The rival responds and leads the player to a nearby safe staging point.
5. Both cars stop side-by-side.
6. A real-time cinematic introduces the rival and wager.
7. The player chooses cash, reputation, or — when eligible — a pink-slip race.
8. The player confirms, both engines rev and the countdown runs.
9. The race starts without loading or leaving free roam.
10. Police may become involved and the race continues.
11. The result persists. A rival who wins the player's car may later be seen driving it.

The project is deliberately designed as a **world system**, not a collection of fixed races.

## First supported game target

The first target is NFSMW PC v1.3. Development is being tested against the executable currently used by the project owner:

```text
MD5: C0516B485065FABDD69579816B5DF763
```

Support for additional 1.3 executables will be added only after their addresses/signatures are verified.

## Planned pillars

- Living rivals in free roam
- Natural challenge flow via proximity + horn
- Rival personalities and memory
- Outrun and destination races without loading screens
- Cash, reputation and pink-slip stakes
- Last-car protection: the player can never wager their only car
- Persistent rival garages and economy
- Rivals can potentially drive cars won from the player
- Real-time staging cinematics
- Police integration
- Street meets and rival-vs-rival events
- Safety-first save handling and rollback

## Current status

**Pre-alpha / read-only runtime + executable gameplay-system foundations (v0.0.9-dev).**

The ASI validates the supported executable, installs observation hooks, separates Free Roam from stock races, enumerates live IVehicle driver classes, independently cross-checks the player PVehicle, observes the road network, reads the player's live road-navigation state, reads career cash/car-count/completion, and validates the configured rival-car catalog without mutating the game.

The population layer distinguishes authored persistent rivals, deterministic procedural locals with stable names/personality/vehicle identity, legendary condition-based rivals, and untouched vanilla traffic.

v0.0.8 also turns several design contracts into tested code:

- world-generation tracking so live engine pointers cannot survive a world transition;
- generation-scoped rival runtime handles with destroy-pending invalidation;
- fail-closed spawn environment/candidate gates;
- read-only experimental spawn preflight diagnostics;
- Outrun result/lead/hold/timeout/abort logic;
- staging candidate scoring for straight, wide, streamed, non-junction road sections;
- cinematic staging state flow from search through release;
- mandatory camera/control restoration directives on staging failure or completion.

v0.0.9 adds a guarded, read-only player-road probe through the validated PVehicle/IVehicleAI chain. In Free Roam it samples CurrentRoad, FutureRoad, segment/lane, width, curvature, occlusion, SeekAheadPosition, FarFuturePosition/FarFutureDirection, straight-line distance and forward projection. These values are logged in engine world units so they can be validated in-game before being promoted into spawn or staging coordinates.

The next hard runtime milestone remains intentionally narrow:

> Validate the road-nav probe across Rockport, promote only proven road-relative geometry into an off-screen spawn candidate, then feed that candidate into a dedicated gameplay-thread construction probe and prove create → AI → road navigation → cleanup.

The render callback remains observation-only. Pink slips and save mutation come later, after the runtime foundation is proven stable.

## Repository map

```text
src/                  native ASI source
config/               default configuration templates
docs/
  VISION.md            design principles
  GAME_DESIGN.md       gameplay systems
  TECHNICAL_FEASIBILITY.md
  ARCHITECTURE.md
  ROADMAP.md
  SAVE_SAFETY.md
  RESEARCH_NOTES.md
  ENGINE_INTEGRATION_MAP.md
  SPAWN_AND_AI_PLAN.md
  RUNTIME_SPAWN_CONTRACT.md
  ROAD_NAV_PROBE.md
  PINK_SLIP_ENGINE_RESEARCH.md
  RIVAL_POPULATION_AND_VEHICLES.md
.github/workflows/     CI build
```

## Building

The project uses pinned versions of `nfsmw-2005-sdk`, `MWSDK`, and `NFSPluginSDK`; each serves a different validated layer described in `docs/ENGINE_INTEGRATION_MAP.md`.

Requirements:

- CMake 3.20+
- Visual Studio 2019/2022 with Desktop C++ workload
- Win32/x86 target
- Internet access on first configure so CMake can fetch the SDK

From an **x86 Native Tools** prompt:

```bat
cmake -S . -B build -A Win32
cmake --build build --config Release
```

NFSMW is a 32-bit game, so a 64-bit DLL will not load.

The generated `.asi` belongs in:

```text
<Need For Speed Most Wanted>\scripts\
```

## Important safety rule

The project will **not** directly edit or rewrite the career save for pink-slip transfers until a safe, verified engine-backed transaction path exists.

A pink-slip system must be transactional:

```text
validate -> snapshot -> transfer -> verify -> commit
                              \-> rollback on failure
```

See `docs/SAVE_SAFETY.md`.

## External research basis

This project is being built using public clean-room reverse-engineering/modding work, especially:

- https://github.com/s-b-repo/nfsmw-2005-sdk
- https://github.com/s-b-repo/nfsmw-2005-re
- https://github.com/TsyVM/MWSDK
- https://github.com/TsyVM/MWEncyclopedia
- https://github.com/berkayylmao/NFSPluginSDK
- https://github.com/berkayylmao/NFS-Chat-Chaos-Mod

No EA game assets are stored in this repository.
