# Changelog

## v0.0.44-dev — live rival locator, guarded retirement/retry, reset diagnostics

- The 2026-10-10 owner logs show v42 GTI generated about 74m away and autonomously drove far from the player without a locator; a later v43 spawn about 23m away failed at native road reset, was not activated and entered deferred cleanup.
- On the existing D3D HUD, show straight-line distance to the VALIDATED live owned GTI. Show direction relative to current player movement while driving; when stationary, explicitly show direction unavailable. This is not a road route, map marker or position across invalid world/streaming.
- Add per-second playerDistanceMeters to prototype observation logs.
- Log specific road reset failure guard (vehicle state, AI pointer/vtable, seed, native return, snapshot or position) while retaining all native addresses, guards and call order.
- Only the play preset opts into hold Ctrl+Shift+R (virtual key 0x52 with Ctrl+Shift and 1.5s) to request safe native removal. Default INI continues to disable manual retirement. Removal may be deferred until pursuit-clear, hidden and >=300m away.
- After a mod-requested retirement, F8 re-arms only when BOTH native registries have excluded the owned car in two different completed gameplay frames, with no factory fault and no surviving owned/pending native identity. Native faults, transitions, and engine-side disappearance do NOT re-arm the prototype.
- Add domain locator tests. Full in-game verification of HUD, native cleanup, road reset and repeated create/remove remains pending.


## v0.0.43-dev — native clock and challenge feedback

- Real v42 capture: 19 G edges; one battle begins then aborts after 0.259s. History correctly stores one abort. No mod cleanup or confirmed engine retirement; some transitory world gaps recover. Disappearance cause remains unresolved.
- Correct proven ABI/time mistake: native 0x663D30 uses FILD int32 and multiplies by 1/65536 and 0.001f, not float seconds. Forward raw integer stack word unchanged to original; convert only FRR callback time. Preserve all mutation/source/thread gates.
- Reproduce old false teleport rejection on ordinary 13m/0.25s movement; validate converted time and native original-argument/recursive callback behavior. Freeze scoring on nonadvancing physical poses with stale speed.
- Disable manual retirement by default and remove F7 polling; optional key/modifiers/hold configuration retained. No default replacement shortcut assumed conflict-free.
- G refusals show actionable feedback and log reasons. Race interruptions log reason and terminal sample. Horn source, streaming recovery and target-game confirmation remain pending; no safety gate relaxed.

## v0.0.42-dev — integrated live outrun

- Preserve v41 native driving path; real capture proves GTI activation and autonomous movement, but reported disappearance remains unexplained (F8 retirement requested, never confirmed).
- F8 creates only; hold F7 for 1.5 seconds to request guarded retirement. No result-triggered removal or cleanup caused only by temporarily unavailable active observations.
- Connect G acceptance to live owned rival, native update time, 300m/3s outrun scoring, bounded observed leader trail and close verified overtakes. Block new battles during uncertain/active pursuits; accepted scoring continues without pursuit/AI writes. Abort on world changes, teleports, incompatible routes or invalid telemetry.
- Add mod-owned D3D9 bitmap HUD from existing EndScene hook, full state capture/restore, no reset-sensitive resources. Present never draws it.
- Persist wins/losses/draws/aborts by hashed profile in separate atomic mod files. Corrupt, mismatched, stale and conflicting history is preserved; vanilla saves, cash and garage remain untouched.
- Reject nonfinite/malformed numeric settings and honour General.Enabled=0. Include an enabled play preset without silently changing existing INIs.
- Add meaningful portable route/lifecycle/config/store regression suites and EndScene-only callback checks. New in-game challenge/rendering/retirement behavior remains unvalidated until target capture. Native roaming AI does not follow a player-selected race route.

## v0.0.41-dev — scoped construction confirmation

- Real v40 capture: all 11 native signatures pass; constructor returns GTI handle 34, then full-fleet cross-frame preservation faults before any AI preparation or activation. No selected first-chance exception was recorded. Unknown which previous entry disappeared.
- Check whole-fleet preservation synchronously after construction, before returning to the game; log and reject constructor-side disappearance. Keep the capacity eviction guard and never retry construction.
- Across completed frames, require both fresh registries to contain the player and pending GTI; verify the GTI vtable/simable/handle/model/non-player identity and both pursuits in each confirmation frame. Unrelated fleet changes are logged rather than treated as loss of GTI ownership.
- Confirm on each completed gameplay callback instead of the 250ms search throttle. Retain two distinct safe frames, timeout and all later activation/retirement guards.
- Portable regression covers unrelated fleet turnover, each missing critical interface, streak reset/recovery, player alias and null identities. Win32 CI runs the ABI, native safety and exception observer tests. Autonomous driving and safe game retirement remain unverified.

## v0.0.40-dev

- Investigate the user-reported pre-world startup exit: two v0.0.39 sessions end before any F8 construction; neither the last logged function nor a loaded mod is a proven culprit.
- Add a process-lifetime native exception observer and bounded, file-backed `NativeExceptions.log`, enabled by default and independent of the normal logger. First-chance native records never suppress or modify an exception. Preserve older INI settings.
- Record exception address, code, registers, thread and FRR callback phase, with an installation module inventory and a latest-128 ring. Keep v0.0.39 construction/confirmation and pursuit guards unchanged.
- Add Windows integration tests including a real fatal PAGE_NOACCESS write in a child process; this validates the observer, not the game or rival driving.
- Document the capture, limitations and isolated Bartender/X360Stuff test. No claim that the startup exit is fixed.

## 0.0.39-dev

- Record the real v38 capture: user saw a stationary Golf GTI; all eleven signatures passed in the final session, constructor was invoked, but preparation/activation was not reached. Earlier sessions still contained the Bartender hook. Driving and retirement remain unverified.
- Retain a successful native return as a pending scalar token and confirm membership in both native registries over two distinct completed gameplay samples before strict identity/deactivation/handle stamping. Bound confirmation to two seconds; reset on incomplete registry, lost membership, changed context or non-clear pursuit. Preserve preconstruction identities and never retry a constructor or adopt an unreturned partial allocation. This addresses a possible readiness window, not a game-proven explanation of the v38 failure.
- Add named fault/identity/operation phases and native SEH code/instruction/access-address diagnostics instead of a silent disabled transition. Keep exact executable/function signatures and road/occupancy/pursuit gates intact.
- Extend portable safety tests for confirmation interruption/repeated frames and Win32 ABI tests for numeric PVehicle/ISimable/IVehicle base conversion. No engine objects are instantiated by tests.

## 0.0.38-dev

- Add opt-in `DiagnosticBundleEnabled=1` preset for existing verified render/input/road/gameplay/collision/camera/motion/post-race observation. Does not enable native construction, AI control, economy or garage writes.
- F9 now independently requests a bounded module/live-disk-code/world audit, without requiring a safe spawn target. No capacity hook or constructor/goal/reset/activate/retirement calls occur in the read-only audit. F8 independently revalidates before construction.
- Log up to 256 loaded modules at the first valid callback and on F9; record enumeration failures/truncation. Guard extra spatial/four-road-batch samples behind clear Free Roam and pursuit; throttle F9 to one request per 10 seconds. This is not an exhaustive world scan.
- Add optional local ZIP collector: bounded log tail, known FRR INIs, binary hashes/inventory and evidence-derived next checks. Never includes game binaries/plugins/saves, uploads, edits configuration or overwrites an existing output.
- Add seven collector regression tests, package collector/launcher and document researched primary sources plus a concrete evidence-gated plan for compatibility, creation, driving, streaming and pursuits. Windows/module/game execution remains unconfirmed here.


## 0.0.37-dev

- v36 PC log confirms 125 construction requests rejected by the live 0x422480 signature. No native constructor invocation occurred. All 11 expected FNV-1a hashes match a fresh offline read of the exact supported executable; the source of the live difference remains unknown.
- Audit all 11 native function windows instead of stopping at the first mismatch. Compare live and disk windows against unchanged expected signatures after the exact executable guard; missing disk/live reads also block.
- For mismatches log first 32 bytes, first differing offset/address and nearby 32-byte windows. Classify entry E9/FF25 jump shapes and resolve target modules where possible without claiming that shape proves a foreign hook.
- Halt prototype attempts for the session after the complete failed audit, before capacity-hook installation or constructor calls. Distinguish construction requests from actual constructor invocations in logs.
- Add portable fingerprint, window-difference, entry-shape and exact supported .text file-offset boundary regressions. No relaxed compatibility signature and no in-game rival success claimed.


## 0.0.36-dev

- Add explicit `NativeRivalPrototypeNearPlayer=1` manual F8 test: visible creation/activation at verified free road targets 20–120 metres away. Both prototype flags default to off.
- Preserve ground, full fleet clearance, metric calibration, world/player/model, pursuit/cooldown, stable-window and population/capacity checks. Cleanup still requires hidden/300m conditions.
- Record debug mode, measured distance and position at construction; prefer closer eligible targets within each capture batch. Do not invent player-relative road seeds or claim hidden/streaming proof.
- Reject non-finite metric distances and test debug interval boundaries plus unchanged normal and pursuit/occupancy policies.
- v35 PC log reached hidden-camera/ray rejection after ground checks but contains no native construction. Real GTI appearance/driving remains unconfirmed.


## 0.0.35-dev

- v34 PC evidence confirms 5–10 road targets captured, but ground/clearance gates rejected creation; no native construction occurred.
- Fix the prototype ground-ray direction: native collision normals face the ray origin, so the old upward fallback conflicted with the required upward normal. Use a local downward Y+2 to Y-4 ray only for the prototype; retain all ground/height/slope/clearance/visibility/pursuit gates.
- Add bounded rejection detail for actual ground calls and a flat-face direction regression with height/slope/barrier/no-hit rejection checks. Tests do not run the game.
- Ignore repeated F8 presses while seeking instead of immediately cancelling the request and resetting readiness. Native cleanup behavior after construction is unchanged.
- Creation, activation, movement and safe lifecycle remain unconfirmed in the game.

## 0.0.34-dev

- User logs confirm enabled F8 requests but zero captured navigation targets and no construction attempt. The earlier startup closure has no recorded cause.
- Include the AI's existing DriveToNav as a scalar-only road source: verify native slot 18/getter 0x431C50, read IVehicleAI+0x24 directly and recheck the pointer. Do not invoke UpdateRoads or mutate the source vehicle.
- Report source-capture early exits and rejection counts rather than presenting every zero-target result as a distance failure.
- Skip additional native factory/world sampling while the prototype is idle/finished/disabled; rebuild readiness after F8. This is not a confirmed crash fix.
- Extend coverage tests to up to 48 captured targets per batch and assert the native DriveToNav member offset. In-game creation/activation remains unconfirmed.

## 0.0.33-dev

- Rotate native road-source batches (16 live slots maximum per sample) instead of stopping at the first 32 captured targets.
- Give each source batch its own rotating four-candidate evaluation window so earlier rejected candidates do not permanently exclude later targets.
- Preserve detailed safety rejection reasons and log candidate-search counts once per second. All distance, pursuit, identity, clearance, ground and hidden-position gates remain unchanged.
- Add coverage tests for all 32 source batches, shared population/budget divisors, empty populations and population shrinkage. Actual-game appearance/driving and lifecycle safety remain unvalidated.

## 0.0.32-dev

- Connect an explicit opt-in F8 native one-GTI prototype to the verified outer gameplay post-update callback; one construction per session, disabled by default.
- Capture a pointer-free scalar road seed from a freshly registered native AI, reset the new racer's own navigation, recheck actual body/ground/fleet clearance and use SetSpawned -> native Racer goal -> Activate.
- Require exact executable, fresh pursuit/cooldown state, motion metric calibration, 350-850m distance, primary-frustum exclusion and eight blocked world-face rays. Retire only the owned native simable when hidden/300m away and pursuit clear; wait for both registries to omit it.
- Correct render-side road diagnostics: native road/future getters call UpdateRoads and are not read-only. Read verified embedded prefixes instead; do not infer the native future-nav address from the incomplete SDK WRoadNav size.
- Add owned road-seed encoding and malformed-input tests plus expanded compiled native ABI assertions. Actual-game creation, movement, visual streaming and cleanup validation remain pending.

## 0.0.31-dev

- Fix the loss of numeric race evidence during the short race-end fade observed in the actual v30 PC capture. Retention is limited to ten seconds, fresh guards and context checks remain required, and all resumed matches are explicitly interrupted/lifetime-unproven.
- Log observed racer count, retained cohort count and fade retention to distinguish an empty archive from failed correlation.
- Block spawn preflight for unknown/active/cooldown/busted pursuit states. The runtime pursuit reader is not verified; it stays Unknown. This is a conservative policy, not implemented rival police AI or a native reader.
- Add regression coverage for the captured transition, expiry, context changes, clock gaps and pursuit policy.

## 0.0.30-dev

- Deepen pinned-source roaming/streaming/cache research: separate racer goal/class, candidate-section activation, mutating road-nav queries, deferred cleanup and companion resource limits.
- Reject post-race samples when reread live-list storage/count/membership changes; include inactive slots, log registry status and world-transition revocations, and add portable regression cases. Equal reads do not claim atomicity or object lifetime.

- Add disabled-by-default, read-only post-race racer diagnostics on the verified completed gameplay loop, with bounded identity correlation and invalidation on incomplete samples, loading, context changes or observation gaps. No runtime target-game validation has been performed for this new probe.
- Add portable regression tests for transition matching, movement, AI identity changes, disappearance/reuse, invalid observations and evidence expiry.
- Document pinned public reconstruction sources and the independently authored Native Free Roam Racers mod. Treat the user's post-race observation as a research lead, not an executable ABI proof.
- Harden the imported executable dump utility: refuse input/report overwrite, respect raw section boundaries and replace byte-scanned return guesses with optional decoded candidates that do not prove the ABI.
- Correct root-only local-copy ignore rules so new src/game sources are included in Git.

- Record the v0.0.29 target audit: metric calibration verified (1.00159 world units/m), ground=valid with plausible delta/grade, selected GTI footprint verified; remaining blocker SpawnCandidateUnverified (remote road association, streaming, full-view invisibility).
- Add docs/SPAWN_FACTORY_RESEARCH.md: SDK evidence and red flags for PVehicle::Construct (0x689820 is a generic Sim factory taking VehicleParams by value).
- Add docs/TARGET_CAPTURE_V29_ALL_DIAGNOSTICS_2026-10-07.md (audit of the v0.0.29 target log) and docs/EXE_DUMP_V2_FINDINGS.md (static ABI findings with reproducibility hashes).
- Correct CreateAIGoalRacerInstance entry to 0x43D330 in docs (0x43D388 is interior).
- Import Claude's tools/dump_exe_functions.py and research documents; their executable-dump and v0.0.29 target-log claims were supplied without original inputs and have not been independently reproduced in this review.

## 0.0.29-dev

- Confirm the v0.0.28 live-IVehicle spatial fix on target: safe Free Roam samples track the actual live vehicle count with zero failed spatial reads.
- Correct the remaining coordinate semantic mismatch. NFSPluginSDK exposes MW vectors through y,z,x member declarations while reconstructed MW05 PC UMath uses x,y,z with Y vertical.
- Canonicalize player motion, road navigation, live-vehicle positions/bases and rigid-body dimensions at the SDK boundary into MW X/Y-up/Z.
- Restore the raw CheckHitWorld engine mirror to x,y,z,w. Vertical ground queries, grade normals, OBB axes/extents and the camera swizzle now share one simulation convention.
- Add fail-closed world-metric promotion. Source-backed MW05 speed semantics establish GetAbsoluteSpeed as metres/second; target motion still needs at least 20 stable samples, <=2% CV and 0.98-1.02 cross-channel agreement.
- Feed verified metric scale into road-candidate diagnostics and mutation-readiness reporting. Final spawn-candidate proof remains false.
- Keep selected rival model proof strict: Rico's selected GTI footprint is not substituted with another learned model.
- Construction, AI takeover, economy and garage writes remain disabled.

## 0.0.28-dev

- Audited the all-diagnostics v0.0.27 target run: gameplay/render hooks remained healthy for more than five minutes, 24 fallback-key edges were captured, motion telemetry survived four Free Roam generations, and the game stayed fail-closed through transitions.
- Found a concrete world-collision ABI bug: NFSPluginSDK MW05 UMath logical vectors are stored physically as y,z,x,(w), while the raw CheckHitWorld bridge encoded x,y,z,w. This explains repeated completed occlusion queries with every ground result still unverified.
- Fix the raw UMath::Vector4 byte layout for both CheckHitWorld segment input and WorldCollisionInfo vector output; retain the reconstructed 0x58-byte result layout and gameplay-thread mailbox.
- Replace spatial occupancy iteration over the long-lived PVehicle instance pool with MWSDK's verified live IVehicle list. The target run exposed the old mismatch clearly (74 pool entries versus 20 live vehicles, with 53 failed spatial reads).
- Use only IVehicle/ISimable/IRigidBody read calls from live-list entries; no guessed pointer subtraction, construction or state mutation is introduced.
- Keep camera evidence conservative. The primary camera was coherent intermittently on target, but selected-GTI footprint, complete displayed-view coverage, streaming and final visibility remain unverified.
- Update package/release metadata to v0.0.28-dev. Rival construction, AI takeover, economy and garage writes remain disabled.

## 0.0.27-dev

- Audited the supplied v0.0.26 target log: the unique MW05 main-loop call site is present, but its destination is redirected; gameplay callbacks/FrameTicks/G edges therefore correctly remained zero.
- Correlated that redirect with the pinned WidescreenFixesPack implementation, which patches the same CALL to `NFSMostWanted.WidescreenFix.asi!MainLoop(float)` and forwards to the original.
- Add a fail-closed route policy: direct pinned GameFrameTick remains accepted; a redirect is accepted only when the executable target is owned by the exact `NFSMostWanted.WidescreenFix.asi` module. Unknown/missing/ambiguous routes remain blocked.
- Hook the recognized existing wrapper entry rather than bypassing or rewriting its chain; preserve the float argument, original call order, recursion suppression and thread-consistency revocation.
- Add route-policy regressions and explicit target-owner/authorization logging for the next machine test.
- Record target proof that corrected spatial boxes can produce complete zero-failure fleets and verified learned models; later partial spatial failures and the missing selected-GTI footprint remain fail-closed.
- Update package/release metadata to v0.0.27-dev. Vehicle construction, AI, world-collision mutation path, physical metric and economy/garage writes remain disabled.

## 0.0.26-dev

- Recorded target render recovery: 350 motion records, 288 accepted pairs, no auditor mismatches. Physical metric remains unverified.
- Replaced the unobserved SDK input-poller adapter with a guarded, unique MW05 CALL/ABI/pinned-target main-loop bridge for the fallback key. Native input-poll counts remain separate.
- Preserve original cdecl float arguments, publish trampolines before activation, suppress nested callbacks and revoke callback delivery permanently on observed thread changes.
- Add focused-process fallback key sampling and rising-edge logs. No encounter dispatch or native horn is claimed.
- Add an explicit verified/completed main-loop ownership path to readiness and opt-in world-collision queries; retain current-thread, world identity and expiry gates.
- Fix live spatial boxes always rejecting themselves due to an initially false valid flag; use a tested geometry factory without weakening geometry/fleet validation.
- Add portable discovery/readiness/spatial regressions and native Win32 hook tests. Add bounded release publication retries after the prior GitHub HTTP 500.
- Update package to v0.0.26-dev; construction, AI, physical metric and final candidate promotion remain blocked.

## 0.0.25-dev

- Investigated the supplied v0.0.24 target log: supported executable and successful ASI loading, but zero render/input callbacks at health and no motion capture records. Carrera GT/Cobalt SS/tuning/KPH are user notes, not captured scale evidence.
- Replaced one-shot unchecked EndScene vtable installation with guarded EndScene/Present method-entry hooks and bounded per-target original trampolines.
- Added independent MW05 Reset-signature device discovery with image bounds/ambiguity rejection, explicitly labeled pinned-SDK fallback and following device/vtable changes.
- Added Present observation fallback, EndScene duplicate suppression, device scoping and serialized delivery. Preserves all original COM arguments/HRESULTs and catches cached method entry pointers.
- Log actual INI path/switches, method installation status/module, first delivered sample and continuing health after 8 seconds/every 30 seconds.
- Added portable discovery/routing tests and native Win32 MinHook tests for cached calls, replacement devices and unchanged original method behavior.
- Updated install instructions and v0.0.25-dev packaging. Gameplay input/FrameTick, world-collision ownership and all construction gates remain unchanged.

## 0.0.24-dev

- Added opt-in `MotionCaptureEnabled=0` by default: per-sample, round-trip float telemetry with capture/cohort identity and exact player model key.
- Split motion statistics across model/player/profile/race-status/road-network/world identity changes.
- Bounded statistical evidence to the latest 120 consecutive accepted pairs; any rejected pair revokes prior stability.
- Reject nonfinite motion channels, invalid velocity magnitudes and non-forward speed pairs instead of allowing misleading zero cross-checks.
- Added an independent standard-library Python auditor for capture logs, timing/window/ratio consistency, malformed samples and direction-change warnings. It never promotes metric calibration.
- Added C++ motion regressions and Python capture tests; included the auditor and capture instructions in the install ZIP.
- Pinned physical-unit research for HUD MPS conversions, wheel dimensions and drivetrain speedometer calculations. Wall-clock/Speedbreaker and target executable proof remain outstanding.
- Updated package/release metadata to v0.0.24-dev. Vehicle construction remains blocked.

## 0.0.23-dev

- Added opt-in `CameraFrustumDiagnosticsEnabled`, disabled by default, using guarded reads from the already pinned MWSDK primary-camera API.
- Established simulation-to-render mapping `(z, -x, y)` from the MW05 reconstruction; no platform-specific reconstruction addresses are imported.
- Added coherent-matrix guards: active primary view, rigid view basis, perspective shape, eye/view agreement and View * Projection cross-check.
- Added six-plane whole-OBB classification; plane contact and partial intersection remain potentially visible.
- Added camera tests for all six planes, rotated/partial/contact boxes, translated cameras, invalid matrices, mapping, identity and inactive-view rejection.
- Logged primary-camera diagnostics separately from complete spawn visibility, which remains unverified pending mirror coverage, visual bounds and target-machine validation.
- Documented concrete streaming leads and the missing exact-PC section/activation proof. No streaming approximation or raw unverified function call is introduced.
- Updated package/release metadata to v0.0.23-dev. Vehicle construction remains blocked.

## 0.0.22-dev

- Bound pre-construction SAT evidence to one selected pending procedural rival and its exact runtime model key. Selection does not reroll to find an already learned model.
- Promoted selected-model full-fleet results to `RoadCandidateEvidence.overlapVerified`; unknown/incomplete evidence remains occupied/unverified.
- Use maximum observed half-extents instead of the smaller mean for candidate boxes. Later inconsistent measurements revoke readiness.
- Added value-only world evidence leases: generation, both player identities, road network, race status, pseudonymous profile and 500 ms age.
- Collision requests are revalidated with a fresh GameBridge read on the confirmed gameplay thread, replaced by newer samples, and discarded on transitions/expiry. Results also expire.
- Reset footprint/motion accumulators across world generations; readiness observations now reflect the latest safe sample instead of session-long latches.
- Added evidence regression tests and v0.0.22 release packaging. Vehicle construction, metric scale, streaming and final candidate promotion remain blocked.

## 0.0.21-dev

- Added opt-in `WorldCollisionDiagnosticsEnabled` (disabled by default).
- Verified `WCollisionMgr::CheckHitWorld @ 0x7854B0` against a public MW05 mod that explicitly targets the same RELOADED 1.3 MD5 used by Free Roam Rivals.
- Added a conservative raw `WorldCollisionInfo` layout matching the reconstructed 0x58-byte MW05 structure instead of relying on a conflicting abbreviated SDK declaration.
- World-collision calls never run from D3D9 EndScene.
- Added an SRW-lock mailbox:
  - render sampling queues an exact road candidate
  - input/gameplay consumes it only after FrameTick and input thread IDs match
- Added vertical primitive-mask-1 world-face probing based on the reconstructed MW05 rigorous-ground fallback.
- Added pure ground interpretation:
  - ground hit verification
  - raw candidate-to-ground height delta
  - normalized collision normal
  - dimensionless absolute grade
- Added world/barrier line occlusion probing with primitive mask 3.
- World occlusion is deliberately **not** promoted to camera visibility.
- Added `GroundEvidenceUnavailable` to construction readiness.
- Added systems tests for flat ground, sloped ground, barrier rejection, blocked/clear world lines and failed-query fail-closed behavior.
- Camera visibility and streaming remain unresolved and fail-closed.
- Runtime vehicle construction remains disabled.


## 0.0.20-dev

- Added stable per-model footprint identity through `IVehicle::GetVehicleKey()`.
- Added safe catalog-name -> live pvehicle-key resolution.
- Added `VehicleFootprintLearner` with four-sample consistency verification.
- Learns local rigid-body half-extents per vehicle model without constructing a new vehicle.
- Rejects inconsistent dimension samples by relative spread.
- Added road-aligned pre-construction OBB generation using candidate position/forward.
- Runtime can now perform full OBB-vs-live-fleet overlap checks for a verified learned catalog model before vehicle creation.
- Added `VehicleFootprintUnavailable` to construction readiness.
- Expanded runtime diagnostics with learned-model counts, verified model key and pre-construction overlap state.
- Candidate construction remains disabled; metric, streaming, ground and visibility gates remain unresolved/fail-closed.
- Added tests for footprint learning, unstable samples and road-aligned pre-construction overlap.


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

