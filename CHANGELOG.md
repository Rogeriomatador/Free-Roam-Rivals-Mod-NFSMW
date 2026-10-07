# Changelog

## 0.0.9-dev

- Added a real mod-owned post-career Underground Blacklist domain.
- Unlocks only after the vanilla career-completed flag is true.
- Keeps the original Blacklist untouched.
- Added ranked entry states:
  - Locked
  - Rumored
  - HuntAvailable
  - Discovered
  - ChallengeReady
  - Defeated
- Added independent discovered/defeated masks for persistent progression.
- Added current-target world-spawn and challenge eligibility outputs.
- Added Street Rep and qualifier-win requirements per rank.
- Ranked showdowns must still be found and started physically in Free Roam.
- Added stable portrait/intro-voice/defeat-voice/theme asset keys for later media.
- Added 10 provisional post-career ranks (#10 -> #1) in UndergroundBlacklist.ini.
- Added automated Underground Blacklist tests.
- Documented the native FNG frontend target (FRR_UndergroundBlacklist.fng).
- Researched non-destructive frontend integration via the FNG screen stack and targeted FrontB patching.
- No FRONTB/LANGUAGES files are overwritten by this release.
- Runtime vehicle construction, AI mutation, frontend mutation, economy writes and garage transfers remain gated.


## 0.0.8-dev

- Added explicit world-generation tracking for Free Roam runtime sessions.
  - leaving safe Free Roam invalidates the active session
  - re-entering creates a new generation
  - player/road-network identity changes also force a new generation
- Added generation-scoped rival runtime handles.
  - live IVehicle/PVehicle/AI addresses are session-only
  - destroy-pending handles cannot be used
  - invalidation clears all engine addresses
- Turned the documented spawn contract into executable fail-closed rules.
  - supported executable
  - stable Free Roam observation window
  - player + independent PVehicle cross-check
  - road-network availability
  - population budget
  - vehicle/candidate/ground validity
  - overlap rejection
  - hidden/off-screen requirement
  - distance and streaming proof
- Added read-only experimental spawn preflight logging.
  - no vehicle is constructed from D3D9 EndScene
  - enabling the switch only proves environment readiness in this build
- Added a pure Outrun race core with:
  - signed lead tracking
  - player/rival leader reporting
  - held-distance victory
  - timeout resolution
  - draw and abort paths
  - HUD-ready hold progress
- Added a staging candidate scorer/selector that rejects:
  - junctions
  - unstreamed/invalid road
  - obstructions
  - insufficient width
  - excessive curvature/grade
  - invalid ground/two-car geometry
- Added the cinematic staging state machine:
  - Search
  - Reserve
  - Approach
  - Align
  - CameraIntro
  - Negotiating
  - Ready
  - Countdown
  - Release
  - Completed/Aborted
- Added safe hidden-alignment fallback requests and mandatory camera/input restoration directives.
- Expanded INI parsing for rival, experimental, Outrun and staging settings.
- Added a second automated systems test suite.
- Runtime vehicle construction, AI mutation, economy writes, garage writes and pink-slip transfers remain disabled until their engine lifecycle is verified.

## 0.0.7-dev

- Added deterministic procedural rival identity generation.
- Procedural locals now generate a stable bundle from one seed:
  - rival id
  - display name
  - vehicle
  - visual archetype
  - personality
  - challenge style
  - starting cash
  - visual/performance seeds
- Added promotion from ephemeral local to persistent rival.
- Added progression-aware live-rival population budgets.
- Added personality-driven challenge-style selection.
- Preserved the rule that ordinary traffic remains untouched.
- Added automated tests for deterministic identity, persistence promotion,
  legendary exclusion and Rockport Legend population limits.
- Continued engine research on Racer construction, AIGoalRacer, road navigation
  and safe cleanup. Runtime spawning remains disabled until the full lifecycle
  is verified in-game.

## 0.0.6-dev

- Added hybrid rival population design:
  - authored persistent rivals keep a real garage/active car
  - procedural local racers are progression-weighted
  - vanilla civilian traffic remains untouched
- Added deterministic weighted vehicle selection with:
  - career/progression tier ceilings
  - district affinity weighting
  - soft immediate-model anti-repeat
  - explicit legendary/special gating
- Added a vanilla MW05 rival vehicle catalog using community-verified pvehicle keys.
- Added editable VehiclePools.ini design.
- Added one-time live Free Roam catalog validation against the loaded pvehicle database.
- Added authored rival vehicle policies and persistent-garage fields.
- Added automated tests for:
  - early-career tier limits
  - legendary exclusion
  - deterministic seeds
  - anti-repeat
  - legendary opt-in
- Runtime state mutation is still disabled by default while the create/AI/cleanup lifecycle is being proven.

## 0.0.5-dev

- Integrated three pinned research/runtime layers:
  - nfsmw-2005-sdk for hooks and diagnostics
  - MWSDK for verified live IVehicle/world/road data
  - NFSPluginSDK for typed MW05 race/career/PVehicle structures
- Added Free Roam vs stock-race classification using GRaceStatus::mPlayMode.
- Added world-state, road-network, NIS, loading and fade gating.
- Added live IVehicle classification by Human/Traffic/Cop/Racer/NIS/Remote.
- Added independent NFSPluginSDK PVehicle player cross-check.
- Removed an unsafe IVehicle-to-PVehicle pointer-offset conversion after MWSDK
  documentation explicitly warned that registry elements are interface pointers.
- Added read-only career probe:
  - cash
  - current car handle
  - career car count
  - career-completed flag
- Added runtime capability gates. Spawn/economy/garage writes remain false until
  their full lifecycle and rollback paths are verified.
- Added engine integration, spawn/AI and pink-slip research documents.
- External SDK commits are pinned for reproducible builds.
- No rival spawning, AI mutation, cash mutation or garage mutation is enabled.

## 0.0.4-dev

- Added the first real runtime hook after executable validation.
- Hook is read-only and runs after the game's per-frame action polling.
- Added a guarded GameBridge probe for:
  - live PVehicle instance count
  - player-car PVehicle detection
  - AI-car PVehicle count
  - NIS state
  - fade/transition state
  - GRaceStatus pointer presence
  - diagnostic-only raw game-flow value
- Added state-change logging plus periodic heartbeat.
- Added INI controls for the runtime probe.
- Added a pure rival EncounterStateMachine:
  - Roaming
  - Interested
  - ChallengeAvailable
  - Accepted
  - Cooldown
- Added automated tests for the encounter flow.
- Corrected the internal plugin version string.
- Still no spawning, AI control, save writes, economy, garage mutation or pink-slip transfer.

## 0.0.3-dev

- Added formal post-career progression model.
- Added Rockport Legend endgame design.
- Added Street Rep rank model.
- Added pure domain logic for career-aware rival tiers.
- Added pure stake-eligibility rules.
- Enforced last-car protection at the domain-rule level.
- Added safe pink-slip gating requirements.
- Added mixed car + cash stake feasibility logic.
- Added dedicated cinematic-staging design.
- Expanded runtime configuration for progression, world director and wager safety.
- Added Win32 domain tests to CI.
- Gameplay hooks and save mutation remain disabled.

## 0.0.2-dev

- Added executable identity guard.
- Calculates the running speed.exe MD5 and file size at startup.
- Current supported target:
  - size: 6,029,312 bytes
  - MD5: C0516B485065FABDD69579816B5DF763
- Added fail-closed behavior for unsupported executables.
- Split logging and compatibility code into core modules.
- Gameplay hooks and save mutation remain disabled.

## 0.0.1 - bootstrap

- Defined project vision and gameplay pillars.
- Added feasibility and architecture documentation.
- Added safety policy for future pink-slip transactions.
- Added default rival/config templates.
- Added minimal native ASI bootstrap.
- Added Win32 GitHub Actions build workflow.
- No gameplay hooks or save mutations yet.
