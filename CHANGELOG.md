# Changelog

## 0.0.19-dev

- Added read-only `VehicleSpatialProbe` over the validated NFSPluginSDK PVehicle registry.
- Samples active vehicle rigid-body:
  - world position
  - right/up/forward basis
  - local collision dimensions
- Represents live vehicles as oriented boxes.
- Added pure-domain OBB validation and 15-axis SAT intersection testing.
- Added point-in-OBB and point-to-OBB separation queries in world units.
- Fleet evidence fails closed when the registry is incomplete or any active vehicle spatial read is invalid.
- Runtime heartbeats report live spatial-read health.
- Road-candidate heartbeat diagnostics now include point occupancy and nearest live-vehicle separation.
- Added `VehicleSpatialEvidenceUnavailable` to construction readiness.
- Public MW05 reconstruction evidence supports treating rigid-body `GetDimension()` as local half-extents; target runtime logs still sanity-check values.
- Full candidate-footprint overlap is implemented in domain logic but is not promoted at runtime until the selected rival's pre-construction footprint is available.
- Added systems tests for oriented boxes, rotated overlap, clear footprint, occupied footprint and fail-closed incomplete evidence.
- Added `docs/VEHICLE_SPATIAL_EVIDENCE.md`.


## 0.0.18-dev

- Added typed road-candidate observations for CurrentRoad, FutureRoad, SeekAhead and FarFuture.
- CurrentRoad/FutureRoad can carry exact WRoadNav geometry; SeekAhead/FarFuture remain observational until exact road association is independently proven.
- Added explicit candidate blockers for invalid/unavailable geometry, dead ends, points not ahead, unverified metric scale and missing safety evidence.
- Spawn promotion now requires exact road geometry, verified metric conversion, streaming, ground, overlap and verified off-screen visibility.
- Staging promotion requires exact road geometry, verified metric conversion, streaming/ground/overlap plus junction, obstruction, grade and two-car-geometry evidence.
- Kept visibility as a spawn-only pop-in gate; a staging site may be visible while the cars approach it naturally.
- Runtime heartbeat diagnostics now list candidate source, current blocker, segment/lane, world-unit distance and forward projection.
- Added `ExactRoadCandidateUnavailable` to construction readiness.
- Expanded systems tests for observation, metric conversion, spawn promotion, staging promotion and visibility separation.
- Added `docs/ROAD_CANDIDATE_PROMOTION.md`.


## 0.0.17-dev

- Added read-only `PlayerMotionProbe` through the validated player PVehicle.
- Captures:
  - GetSpeed
  - GetSpeedometer
  - GetAbsoluteSpeed
  - GetSlipAngle
  - wheels-on-ground count
  - player world position
  - local velocity + magnitude
  - linear velocity + magnitude
- Added `MotionScaleObserver` using position displacement and elapsed observational time.
- Reports `worldUnitsPerSpeedUnitSecond` rather than pretending the result is metres.
- Cross-checks:
  - speedometer / engine speed
  - absolute speed / engine speed
  - engine speed / local-velocity magnitude
  - engine speed / linear-velocity magnitude
- Rejects samples from unsafe Free Roam, insufficient wheels on ground, very low speed, extreme speed changes and invalid timing/displacement.
- Requires multiple low-variance accepted samples before an observation is called stable.
- Stable motion evidence does not set `WorldMetricCalibration.verified`.
- Runtime heartbeat logs motion-scale evidence and raw motion telemetry.
- Added synthetic systems tests for stable/rejected observations.
- Added `docs/MOTION_SCALE_CALIBRATION.md`.


## 0.0.16-dev

- Added a pure `MutationReadiness` evaluator for the first controlled construction experiment.
- Readiness now identifies the first blocker in a strict sequence:
  - FrameTick probe disabled
  - FrameTick probe not installed
  - FrameTick not observed
  - input polling not observed
  - FrameTick/input thread mismatch
  - safe Free Roam not observed
  - road lookahead unavailable
  - metric calibration unverified
  - spawn candidate unverified
- Runtime sampling records Free Roam/lookahead evidence only through atomics.
- The 8-second hook-health diagnostic now emits a construction-readiness line.
- Matching FrameTick and input-poll thread IDs is required before gameplay-thread evidence is considered confirmed.
- Metric calibration and final candidate promotion remain deliberately false in v0.0.16, so readiness cannot accidentally authorize construction.
- Added systems tests for every blocker transition.
- Added `docs/RUNTIME_READINESS.md`.


## 0.0.15-dev

- Added `ChallengeInputEdge` with deterministic rising-edge semantics.
- Added `ChallengeInputProbe`:
  - sampled from the existing game input-poll callback
  - configurable Windows virtual-key fallback
  - bounded atomic press queue
  - one press can later be consumed by EncounterDirector
  - never injects or overwrites game input
- Added robust decimal/hex parsing for `FallbackChallengeKey`.
- Runtime health diagnostics now report observed fallback challenge presses.
- Runtime explicitly reports that the verified MW05 action map currently exposes no native HORN/HONK action.
- Holding the fallback key cannot generate repeated challenge edges.
- Added systems tests for press/release/rearm behavior.
- Added `docs/CHALLENGE_INPUT.md`.
- EncounterDirector runtime wiring is still pending because no live rival exists yet.


## 0.0.14-dev

- Expanded read-only player road-navigation telemetry with:
  - player position
  - SeekAheadPosition
  - FarFuturePosition
  - FarFutureDirection
  - segment time
  - road/avoidable occlusion counters
  - occluded-from-behind state
  - lookahead distance and forward projection
- Renamed raw road width/span/gap values to explicit `WorldUnits` fields.
- Added `WorldMetricCalibration`.
  - an unverified numeric scale is rejected
  - invalid/non-finite scales are rejected
  - conversion APIs return no value until calibration is explicitly verified
- SpawnSafety now rejects metric candidates with `DistanceScaleUnverified`.
- StagingPlanner now rejects candidates whose metre geometry has not been verified.
- Expanded systems tests for calibration and fail-closed metric use.
- Added `docs/ROAD_NAV_PROBE.md` with runtime evidence and promotion criteria.
- No raw road-nav coordinate is promoted to a live spawn/staging candidate yet.


## 0.0.13-dev

- Added the first-spawn experiment lifecycle state machine.
- The experimental sequence is now encoded as:
  - AwaitConstruction
  - VerifyRegistries
  - VerifyAI
  - ObserveMotion
  - Cleanup
  - VerifyRemoval
  - Succeeded / Failed
- Construction and cleanup commands are one-shot outputs rather than implicit side effects.
- Every phase has a timeout.
- Losing Free Roam/world-generation preconditions before construction fails immediately.
- Losing preconditions after vehicle ownership forces cleanup before final failure.
- A successful experiment still cleans the vehicle up; lifecycle proof requires repeated registry-removal confirmation.
- Cleanup timeout is fatal and disables further session spawning.
- Added dedicated tests for success, construction failure, registry timeout and unsafe-transition cleanup.
- No PVehicle construction call is enabled yet; this controller is the fail-closed orchestration contract the runtime adapter must obey.


## 0.0.12-dev

- Added deterministic Underground Blacklist progress events:
  - Street Rep earned
  - qualifier win
  - pink-slip win
  - current target sighted
  - current target defeated
- Target-specific events enforce the current rank and cannot skip ahead.
- Sighting/defeat events enforce rank requirements.
- A target must be discovered before a defeat result can commit.
- Defeating a rank resets the qualifier-win counter for the next rank.
- Additive counters use saturating arithmetic and reject non-positive event amounts.
- Added explicit rejection reasons for invalid progress events.
- Added `UndergroundBlacklistStore::applyAndSave`:
  - accepted event -> update -> atomic JSON save
  - rejected event -> no write
  - save failure -> event result returned but persistence remains uncommitted
- Expanded Blacklist and persistence tests for ordering, requirements, reset behavior and reload after event commit.


## 0.0.11-dev

- Added an opt-in, read-only mid-hook at verified `GameFrameTick @ 0x663D30`.
- The probe preserves register state and resumes through the SDK/MinHook trampoline.
- Added first-observed thread IDs for:
  - GameFrameTick
  - input polling
  - D3D9 EndScene
- Runtime health logging now reports callback counts and whether FrameTick shares a thread with input/render.
- The probe is disabled by default with `FrameTickProbeEnabled=0`.
- Failure to install is non-fatal and never enables gameplay mutation.
- This is a validation bridge only: spawning/AI still stay disabled until the callback is proven on the target installation.


## 0.0.10-dev

- Added a read-only player WRoadNav probe through the validated PVehicle/IVehicleAI path.
- Runtime snapshots can now observe:
  - player AI pointer availability
  - current and future road-nav objects
  - segment and lane indices
  - road width
  - curvature
  - road start/end span
  - current-to-future navigation distance
- Added a dedicated capability flag for live road-navigation reads.
- Added pseudonymous per-profile identity keys derived in memory from the NFSMW profile name.
  - the raw profile name is never written by Free Roam Rivals
  - only a stable 64-bit hash is used in mod-owned filenames
- Added atomic mod-side Underground Blacklist JSON persistence under:
  - `scripts/FreeRoamRivals/Saves/profile_<hash>.json`
- Persisted fields are limited to mod-owned progression:
  - Street Rep
  - qualifier wins for the current rank
  - pink-slip wins
  - defeated rank mask
  - discovered rank mask
- Runtime-owned facts such as career completion and whether the target is physically present are never trusted from disk.
- Added malformed-schema fail-closed parsing and persistence round-trip tests.
- Runtime automatically binds the correct mod save to the active profile and creates the file after vanilla career completion.
- Vehicle spawning, AI mutation, economy writes and frontend mutation remain disabled pending lifecycle validation.


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
