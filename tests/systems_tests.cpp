#include "domain/ChallengeInput.h"
#include "domain/MotionScaleObserver.h"
#include "domain/MutationReadiness.h"
#include "domain/OutrunRace.h"
#include "domain/RuntimeSession.h"
#include "domain/RivalRuntimeHandle.h"
#include "domain/SpawnSafety.h"
#include "domain/StagingPlanner.h"
#include "domain/StagingStateMachine.h"
#include "domain/WorldMetricCalibration.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void require(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace frr::domain;

    MotionScaleTuning motionTuning{};
    motionTuning.minimumStableSamples = 4;
    motionTuning.maximumCoefficientOfVariation = 0.01f;

    MotionScaleObserver motionObserver(motionTuning);

    MotionScaleFrame motionFrame{};
    motionFrame.valid = true;
    motionFrame.safeFreeRoam = true;
    motionFrame.grounded = true;
    motionFrame.engineSpeed = 10.0f;
    motionFrame.absoluteSpeed = 10.0f;
    motionFrame.localVelocityMagnitude = 10.0f;
    motionFrame.linearVelocityMagnitude = 10.0f;
    motionFrame.x = 0.0f;

    auto motionSnapshot =
        motionObserver.push(motionFrame, 1.0f);

    require(
        motionSnapshot.acceptedSamples == 0,
        "first motion frame establishes baseline only"
    );

    for (int i = 1; i <= 4; ++i) {
        motionFrame.x = static_cast<float>(i * 20);
        motionSnapshot =
            motionObserver.push(motionFrame, 2.0f);
    }

    require(
        motionSnapshot.acceptedSamples == 4 &&
        motionSnapshot.stable &&
        motionSnapshot.meanWorldUnitsPerSpeedUnitSecond > 0.99f &&
        motionSnapshot.meanWorldUnitsPerSpeedUnitSecond < 1.01f,
        "stable synthetic motion observes world-unit to speed-unit ratio"
    );

    require(
        motionSnapshot.meanSpeedToLocalVelocityRatio > 0.99f &&
        motionSnapshot.meanSpeedToLocalVelocityRatio < 1.01f &&
        motionSnapshot.meanSpeedToLinearVelocityRatio > 0.99f &&
        motionSnapshot.meanSpeedToLinearVelocityRatio < 1.01f,
        "motion observer cross-checks engine speed against velocity vectors"
    );

    MotionScaleFrame airborne = motionFrame;
    airborne.grounded = false;
    airborne.x += 20.0f;

    const auto afterAirborne =
        motionObserver.push(airborne, 2.0f);

    require(
        afterAirborne.acceptedSamples == 4 &&
        afterAirborne.rejectedSamples == 1,
        "airborne calibration pair is rejected"
    );

    MotionScaleObserver unstableObserver(motionTuning);
    MotionScaleFrame fast = motionFrame;
    fast.x = 0.0f;
    fast.engineSpeed = 10.0f;
    unstableObserver.push(fast, 1.0f);
    fast.x = 20.0f;
    fast.engineSpeed = 20.0f;

    const auto unstable =
        unstableObserver.push(fast, 2.0f);

    require(
        unstable.acceptedSamples == 0 &&
        unstable.rejectedSamples == 1,
        "large speed swing is rejected from scale observation"
    );

    MutationReadinessInput readiness{};
    auto readinessReport =
        evaluateMutationReadiness(readiness);

    require(
        !readinessReport.readyForConstructionExperiment &&
        readinessReport.blocker ==
            MutationReadinessBlocker::FrameTickProbeDisabled,
        "mutation readiness starts with FrameTick probe disabled"
    );

    readiness.frameTickProbeEnabled = true;
    readinessReport = evaluateMutationReadiness(readiness);
    require(
        readinessReport.blocker ==
            MutationReadinessBlocker::FrameTickProbeNotInstalled,
        "readiness requires installed FrameTick probe"
    );

    readiness.frameTickProbeInstalled = true;
    readinessReport = evaluateMutationReadiness(readiness);
    require(
        readinessReport.blocker ==
            MutationReadinessBlocker::FrameTickNotObserved,
        "readiness requires observed FrameTick calls"
    );

    readiness.frameTickCount = 10;
    readiness.frameTickThreadId = 100;
    readinessReport = evaluateMutationReadiness(readiness);
    require(
        readinessReport.blocker ==
            MutationReadinessBlocker::InputPollNotObserved,
        "readiness requires input-poll evidence"
    );

    readiness.inputPollCount = 10;
    readiness.inputThreadId = 200;
    readinessReport = evaluateMutationReadiness(readiness);
    require(
        readinessReport.blocker ==
            MutationReadinessBlocker::MainLoopThreadUnconfirmed,
        "readiness rejects unmatched FrameTick/input threads"
    );

    readiness.inputThreadId = 100;
    readinessReport = evaluateMutationReadiness(readiness);
    require(
        readinessReport.gameplayThreadConfirmed &&
        readinessReport.blocker ==
            MutationReadinessBlocker::FreeRoamNotObserved,
        "matching FrameTick/input thread confirms gameplay-thread evidence"
    );

    readiness.safeFreeRoamObserved = true;
    readinessReport = evaluateMutationReadiness(readiness);
    require(
        readinessReport.blocker ==
            MutationReadinessBlocker::RoadLookaheadUnavailable,
        "readiness requires live road lookahead"
    );

    readiness.roadLookaheadObserved = true;
    readinessReport = evaluateMutationReadiness(readiness);
    require(
        readinessReport.blocker ==
            MutationReadinessBlocker::MetricCalibrationUnverified,
        "readiness requires verified metric scale"
    );

    readiness.metricCalibrationVerified = true;
    readinessReport = evaluateMutationReadiness(readiness);
    require(
        readinessReport.blocker ==
            MutationReadinessBlocker::SpawnCandidateUnverified,
        "readiness requires final verified spawn candidate"
    );

    readiness.spawnCandidateVerified = true;
    readinessReport = evaluateMutationReadiness(readiness);
    require(
        readinessReport.readyForConstructionExperiment &&
        readinessReport.blocker ==
            MutationReadinessBlocker::None,
        "all evidence gates construction experiment readiness"
    );

    ChallengeInputEdge challengeEdge{};

    require(
        !challengeEdge.update(false),
        "released challenge input has no edge"
    );
    require(
        challengeEdge.update(true),
        "challenge input emits one rising edge"
    );
    require(
        !challengeEdge.update(true),
        "holding challenge input does not repeat"
    );
    require(
        !challengeEdge.update(false),
        "release rearms without emitting"
    );
    require(
        challengeEdge.update(true),
        "second press emits a new edge"
    );

    challengeEdge.reset();
    require(
        !challengeEdge.previousDown(),
        "challenge input reset clears held state"
    );

    WorldMetricCalibration rawScale{};
    rawScale.verified = false;
    rawScale.worldUnitsPerMeter = 1.0f;

    require(
        !worldUnitsToMeters(100.0f, rawScale).has_value(),
        "numeric world-unit scale is unusable until explicitly verified"
    );

    WorldMetricCalibration verifiedScale{};
    verifiedScale.verified = true;
    verifiedScale.worldUnitsPerMeter = 2.0f;

    const auto meters =
        worldUnitsToMeters(100.0f, verifiedScale);

    require(
        meters.has_value() &&
        *meters == 50.0f,
        "verified world-unit scale converts to metres"
    );

    const auto worldUnits =
        metersToWorldUnits(50.0f, verifiedScale);

    require(
        worldUnits.has_value() &&
        *worldUnits == 100.0f,
        "verified metric conversion round-trips"
    );

    RuntimeSessionTracker sessions;
    RuntimeSessionObservation obs{};
    obs.safeFreeRoam = true;
    obs.playerIdentity = 0x1000;
    obs.roadNetworkIdentity = 0x2000;

    auto session = sessions.tick(obs);
    require(
        session.newGeneration &&
        session.generation == 1 &&
        session.stableSamples == 1,
        "session begins generation"
    );

    session = sessions.tick(obs);
    require(
        !session.newGeneration &&
        session.stableSamples == 2,
        "session stability increments"
    );

    obs.safeFreeRoam = false;
    session = sessions.tick(obs);
    require(
        !session.active &&
        session.stableSamples == 0,
        "unsafe transition invalidates session"
    );

    obs.safeFreeRoam = true;
    session = sessions.tick(obs);
    require(
        session.newGeneration &&
        session.generation == 2,
        "re-entry creates generation"
    );

    RivalRuntimeHandle handle;
    RivalRuntimePointers runtimePointers{};
    runtimePointers.iVehicle = 0x3000;
    runtimePointers.pVehicle = 0x4000;
    runtimePointers.vehicleAI = 0x5000;

    handle.bind(
        42,
        session.generation,
        runtimePointers,
        true
    );

    require(
        handle.validForGeneration(session.generation),
        "runtime handle is scoped to world generation"
    );
    require(
        !handle.validForGeneration(session.generation + 1),
        "stale generation invalidates runtime handle"
    );

    handle.markDestroyPending();
    require(
        !handle.validForGeneration(session.generation) &&
        handle.destroyPending(),
        "destroy-pending handle cannot be used"
    );

    handle.invalidate();
    require(
        !handle.bound(),
        "runtime handle clears all live pointers"
    );

    SpawnEnvironmentInput spawnEnv{};
    spawnEnv.experimentalFeatureEnabled = true;
    spawnEnv.supportedExecutable = true;
    spawnEnv.freeRoamCandidate = true;
    spawnEnv.playerAvailable = true;
    spawnEnv.independentPlayerCrossCheck = true;
    spawnEnv.roadNetworkAvailable = true;
    spawnEnv.stableFreeRoamSamples = 6;
    spawnEnv.liveRivals = 0;
    spawnEnv.maxLiveRivals = 1;

    SpawnCandidateInput spawnCandidate{};
    spawnCandidate.available = true;
    spawnCandidate.vehicleAvailable = true;
    spawnCandidate.roadValid = true;
    spawnCandidate.groundValid = true;
    spawnCandidate.visibleToPlayer = false;
    spawnCandidate.metricDistanceVerified = false;
    spawnCandidate.distanceFromPlayerMeters = 500.0f;

    auto spawn =
        evaluateSpawnCandidate(spawnEnv, spawnCandidate);

    require(
        !spawn.allowed &&
        spawn.reason ==
            SpawnRejectReason::DistanceScaleUnverified,
        "raw world-unit distance cannot enter metric spawn thresholds"
    );

    spawnCandidate.metricDistanceVerified = true;

    spawn =
        evaluateSpawnCandidate(spawnEnv, spawnCandidate);

    require(
        spawn.allowed,
        "safe hidden candidate allowed"
    );

    spawnCandidate.visibleToPlayer = true;
    spawn = evaluateSpawnCandidate(spawnEnv, spawnCandidate);
    require(
        !spawn.allowed &&
        spawn.reason == SpawnRejectReason::VisiblePopInRisk,
        "visible spawn blocked"
    );

    spawnCandidate.visibleToPlayer = false;
    spawnCandidate.distanceFromPlayerMeters = 900.0f;
    spawn = evaluateSpawnCandidate(spawnEnv, spawnCandidate);
    require(
        !spawn.allowed &&
        spawn.reason ==
            SpawnRejectReason::TooFarWithoutStreamingProof,
        "far spawn needs streaming proof"
    );

    spawnCandidate.streamingVerified = true;
    spawn = evaluateSpawnCandidate(spawnEnv, spawnCandidate);
    require(
        spawn.allowed,
        "far spawn allowed only with streaming proof"
    );

    OutrunTuning outrunTuning{};
    outrunTuning.winLeadMeters = 100.0f;
    outrunTuning.leadHoldSeconds = 2.0f;
    outrunTuning.maxDurationSeconds = 30.0f;

    OutrunRace outrun(outrunTuning);
    outrun.begin();

    OutrunInput raceInput{};
    raceInput.signedLeadMeters = 110.0f;
    raceInput.deltaSeconds = 1.0f;

    require(
        outrun.tick(raceInput) ==
            OutrunOutcome::InProgress,
        "outrun lead starts hold"
    );
    require(
        outrun.holdProgress01() > 0.49f,
        "outrun hold progress exposed"
    );
    require(
        outrun.tick(raceInput) ==
            OutrunOutcome::PlayerWon,
        "outrun player wins after held lead"
    );

    outrun.begin();
    raceInput.signedLeadMeters = -110.0f;
    raceInput.deltaSeconds = 1.0f;
    require(
        outrun.tick(raceInput) ==
            OutrunOutcome::InProgress,
        "rival lead starts hold"
    );

    raceInput.signedLeadMeters = 0.0f;
    require(
        outrun.tick(raceInput) ==
            OutrunOutcome::InProgress &&
        outrun.holdSeconds() == 0.0f,
        "lead loss resets hold"
    );

    StagingCandidate bad{};
    bad.metricGeometryVerified = true;
    bad.roadValid = true;
    bad.streamed = true;
    bad.groundValid = true;
    bad.supportsTwoCars = true;
    bad.distanceAheadMeters = 100.0f;
    bad.roadWidthMeters = 8.0f;
    bad.junction = true;

    StagingCandidate good = bad;
    good.junction = false;
    good.distanceAheadMeters = 120.0f;
    good.roadWidthMeters = 9.0f;
    good.absoluteCurvature = 0.005f;
    good.absoluteGrade = 0.02f;

    StagingCandidate acceptable = good;
    acceptable.distanceAheadMeters = 190.0f;
    acceptable.roadWidthMeters = 7.2f;
    acceptable.absoluteCurvature = 0.015f;

    StagingCandidate uncalibrated = good;
    uncalibrated.metricGeometryVerified = false;

    require(
        !scoreStagingCandidate(uncalibrated).eligible,
        "uncalibrated world geometry cannot enter metre-based staging score"
    );

    const std::vector<StagingCandidate> candidates = {
        bad,
        acceptable,
        good
    };

    const auto best =
        selectBestStagingCandidate(candidates);

    require(
        best &&
        *best == 2,
        "staging planner chooses safest straight candidate"
    );

    StagingTuning stagingTuning{};
    stagingTuning.alignmentTimeoutSeconds = 1.0f;

    StagingStateMachine staging(stagingTuning);
    StagingInput stagingInput{};
    stagingInput.stagingCandidateFound = true;
    staging.tick(stagingInput);

    require(
        staging.state() == StagingState::Reserve,
        "staging found candidate"
    );

    stagingInput.reservationValid = true;
    staging.tick(stagingInput);
    require(
        staging.state() == StagingState::Approach,
        "staging reserved candidate"
    );

    stagingInput.approachComplete = true;
    staging.tick(stagingInput);
    require(
        staging.state() == StagingState::Align,
        "staging enters alignment"
    );

    stagingInput.hiddenAlignmentTransformSafe = true;
    stagingInput.deltaSeconds = 1.1f;
    auto stagingUpdate = staging.tick(stagingInput);

    require(
        staging.state() == StagingState::CameraIntro &&
        stagingUpdate.requestHiddenAlignment,
        "alignment timeout requests hidden safe correction"
    );

    stagingInput.cameraIntroComplete = true;
    stagingUpdate = staging.tick(stagingInput);
    require(
        staging.state() == StagingState::Negotiating &&
        stagingUpdate.cinematicCameraActive,
        "camera intro hands off to negotiation"
    );

    stagingInput.stakeConfirmed = true;
    staging.tick(stagingInput);
    stagingInput.enginesReady = true;
    staging.tick(stagingInput);
    stagingInput.countdownComplete = true;
    staging.tick(stagingInput);
    stagingInput.releaseComplete = true;
    stagingUpdate = staging.tick(stagingInput);

    require(
        stagingUpdate.completed &&
        stagingUpdate.restorePlayerControl &&
        stagingUpdate.restoreCamera,
        "staging completion restores camera and controls"
    );

    staging.reset();
    stagingInput = {};
    stagingInput.worldSafe = false;
    stagingUpdate = staging.tick(stagingInput);

    require(
        stagingUpdate.aborted &&
        stagingUpdate.restorePlayerControl &&
        stagingUpdate.restoreCamera,
        "unsafe transition aborts staging with cleanup directives"
    );

    std::cout
        << "Free Roam Rivals systems tests passed.\n";
    return 0;
}
