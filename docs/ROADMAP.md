# Roadmap

## v0.0.1 — Bootstrap

- [x] Repository structure
- [x] Design documentation
- [x] Minimal ASI scaffold
- [x] Configuration templates
- [x] CI build scaffold
- [x] Confirm ASI build on Win32
- [x] Confirm plugin loads on target executable
- [x] Create runtime log in game folder

Exit criterion: loading the ASI does not alter gameplay and creates a diagnostic log.

## v0.1 — One living rival

- [x] Detect free-roam gameplay state (runtime validation still collected in logs)
- [x] Resolve player IVehicle safely + independent PVehicle cross-check
- [x] Obtain read-only render/input observation callbacks
- [x] Define executable fail-closed spawn gates
- [x] Track world generations and invalidate stale runtime handles
- [x] Add generation-scoped rival runtime-handle model
- [x] Add read-only spawn preflight diagnostics
- [x] Read native SeekAhead/FarFuture road-lookahead geometry
- [x] Block raw world-unit geometry from metric spawn thresholds until calibrated
- [x] Emit a single fail-closed construction-readiness diagnostic
- [x] Instrument GetSpeed/GetSpeedometer/absolute/local/linear motion cross-checks
- [x] Observe world-units per engine-speed-unit-second statistically
- [ ] Capture target-machine motion logs across cars/districts
- [ ] Prove physical GetSpeed unit semantics and calibrate world-units-to-metre scale
- [x] Build typed Current/Future/SeekAhead/FarFuture road-candidate observations
- [x] Require exact WRoadNav association before candidate promotion
- [x] Encode streaming/ground/visibility/overlap evidence required by spawn promotion
- [ ] Implement live streaming/ground/visibility/overlap evidence probes
- [ ] Extract and promote one verified road-safe/off-screen spawn candidate in-game
- [ ] Validate dedicated gameplay-thread mutation callback in-game
  - [x] Add opt-in read-only GameFrameTick entry probe
  - [x] Encode callback/thread/FreeRoam/lookahead/exact-road/calibration/candidate readiness blockers
  - [ ] Confirm callback/thread health on target installation before mutation
- [x] Encode construct/registry/AI/motion/cleanup experiment as a fail-closed state machine
- [ ] Wire the state machine to verified engine construction/cleanup calls
- [ ] Run and pass one controlled construct/verify/clean-up cycle in-game
- [ ] Attach/verify native roaming AI
- [x] Progression-aware vanilla vehicle catalog and deterministic selector
- [x] Persistent-vs-procedural vehicle ownership policy
- [x] Interest/challenge distance state machine (runtime adapter pending)
- [x] Parse configurable fallback challenge key
- [x] Detect edge-triggered fallback challenge input from the game input-poll callback
- [ ] Identify a genuine native horn/honk source (current verified action table has none)
- [ ] Wire queued challenge presses into runtime EncounterDirector
- [ ] Wire accepted challenge into runtime encounter director
- [x] No save writes

Exit criterion: one rival can exist in free roam and naturally enter a challenge-ready state.

## v0.2 — Outrun

- [x] Implement engine-independent Outrun race state
- [x] Signed lead/separation model
- [x] Hold-distance win condition
- [x] Timeout/draw/abort recovery rules
- [x] Race result model
- [x] HUD-ready hold progress
- [ ] Derive signed lead from live road progress
- [ ] Basic D3D9 HUD
- [ ] Wire result into rival cooldown/history

Exit criterion: complete repeatable 1v1 Outrun without loading a stock race.

## v0.3 — Staging + cinematic

- [x] Read road-network singleton
- [x] Read live player current/future WRoadNav segment/lane geometry
- [x] Read native seek-ahead/far-future positions and occlusion evidence
- [x] Require verified metric scale before metre-based staging scoring
- [x] Implement safe staging-candidate scoring/selection rules
- [x] Define typed road-observation -> staging-candidate promotion contract
- [ ] Populate fully evidenced staging candidates from the live road network
- [x] Implement Search -> Reserve -> Approach -> Align state flow
- [ ] Rival approach runtime control
- [ ] Temporary player control suppression bridge
- [x] Alignment timeout + validated hidden-snap fallback request
- [x] CameraIntro -> Negotiating -> Ready -> Countdown -> Release state flow
- [ ] Real-time camera ownership/shot bridge
- [ ] Wager UI bridge
- [x] Mandatory restore-camera/input directives on completion/abort
- [ ] Validate restoration in-game across every interruption

Exit criterion: accepted challenge transitions into a polished side-by-side start.

## v0.4 — Economy

- [ ] Verified read/write cash bridge
- [ ] Cash stakes
- [ ] Insufficient-money UI
- [ ] Rival virtual cash
- [ ] Reputation stake
- [ ] Transaction framework
- [ ] Never write raw save bytes

Exit criterion: repeatable cash wagers that survive save/load without corruption.

## v0.5 — Rival persistence

- [x] Rival definitions (template schema)
- [x] Persistent personality bundle for procedural rivals
- [ ] W/L history
- [ ] Respect / grudge / fear evolution
- [x] Persistent rival garage model/schema
- [ ] Rematches
- [x] District preferences in vehicle selection model
- [ ] Dynamic difficulty without speed cheating

## v0.6 — Pink slip: player wins

- [ ] Verify engine-backed award-car path on supported exe
- [x] Domain eligibility rules
- [x] Last-car rule for rival
- [ ] Add wagered rival car safely
- [ ] Verify garage after transaction
- [ ] Remove car from rival virtual garage only after success
- [ ] Recovery on failure

## v0.7 — Full pink slip

- [ ] Enumerate player-owned cars safely
- [x] Domain last-car protection
- [ ] Identify exact wagered car record
- [ ] Snapshot exact customization
- [ ] Remove/transfer using verified engine path
- [ ] Select fallback player car
- [ ] Rival stores won car
- [ ] Recovery race
- [ ] Full rollback tests

**v0.7 must not ship until save-safety tests pass.**

## v0.8 — Living ecosystem

- [ ] Rival uses cars won from player
- [ ] Rival economy
- [ ] Rival upgrades / vehicle changes
- [ ] Rival actively seeks rematch
- [ ] Street value balancing
- [x] Mixed car + cash stake feasibility logic

## v0.9 — Rockport systems

- [x] Define separate post-career Underground Blacklist domain
- [x] Career-completion unlock rule
- [x] Ranked discovery / rumor / hunt / challenge / defeated states
- [x] Asset-ready portrait and voice keys
- [x] Native FNG frontend feasibility research
- [x] Bind mod-owned Underground Blacklist persistence to a pseudonymous per-profile key
- [x] Atomic JSON load/save + malformed-schema fail-closed handling
- [x] Define validated sighting/qualifier/pink-slip/defeat progress events
- [x] Apply accepted progress events and persist them atomically
- [ ] Wire live race/encounter result sources into the progress-event API
- [ ] World Director integration for current ranked target
- [ ] Native FRR_UndergroundBlacklist.fng screen
- [ ] Add safe menu entry after vanilla career completion
- [ ] Portrait/vehicle presentation asset pipeline
- [ ] Voice/theme audio bridge
- [ ] Police personalities
- [ ] Pursuit continuation
- [ ] Rival may abort due to police
- [ ] Rival-vs-rival races
- [ ] Street Meets
- [ ] Rare / legendary rivals
- [ ] World event scheduler

## v1.0 — Rockport Underground

Release goals:

- stable on supported executables
- no raw save editing
- persistent rivals
- multiple race modes
- polished staging
- cash and pink-slip wagers
- police integration
- safe rollback
- compatibility documentation
