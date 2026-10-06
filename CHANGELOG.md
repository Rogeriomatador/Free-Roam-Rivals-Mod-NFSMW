# Changelog

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
