#include "domain/ChallengeInput.h"
#include "domain/MotionScaleObserver.h"
#include "domain/MutationReadiness.h"
#include "domain/OutrunRace.h"
#include "domain/RoadCandidatePlanner.h"
#include "domain/RuntimeSession.h"
#include "domain/RivalRuntimeHandle.h"
#include "domain/SpawnSafety.h"
#include "domain/StagingPlanner.h"
#include "domain/StagingStateMachine.h"
#include "domain/VehicleFootprintLearning.h"
#include "domain/VehicleSpatialEvidence.h"
#include "domain/WorldMetricCalibration.h"
#include "game/RoadCandidateProbe.h"

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

    VehicleOrientedBox parked{};
    parked.valid = true;
    parked.identity = 0x1111;
    parked.vehicleKey = 0xAABBCCDDu;
    parked.center = {0.0f, 0.0f, 0.0f};
    parked.right = {1.0f, 0.0f, 0.0f};
    parked.up = {0.0f, 1.0f, 0.0f};
    parked.forward = {0.0f, 0.0f, 1.0f};
    parked.halfExtents = {1.0f, 1.0f, 2.0f};

    require(
        validVehicleOrientedBox(parked),
        "orthonormal live-vehicle OBB is valid"
    );

    require(
        pointInsideVehicleOrientedBox(
            {0.5f, 0.0f, 1.5f},
            parked
        ),
        "point inside vehicle half-extents is detected"
    );

    require(
        !pointInsideVehicleOrientedBox(
            {0.0f, 0.0f, 3.0f},
            parked
        ),
        "point beyond vehicle length is outside"
    );

    const float pointSeparation =
        pointSeparationFromVehicleOrientedBox(
            {0.0f, 0.0f, 3.0f},
            parked
        );

    require(
        pointSeparation > 0.99f &&
        pointSeparation < 1.01f,
        "point-to-OBB separation is reported in world units"
    );

    VehicleOrientedBox crossing = parked;
    crossing.identity = 0x2222;
    crossing.center = {2.5f, 0.0f, 0.0f};
    crossing.right = {0.0f, 0.0f, 1.0f};
    crossing.forward = {-1.0f, 0.0f, 0.0f};

    require(
        vehicleOrientedBoxesOverlap(
            parked,
            crossing
        ),
        "SAT detects overlap between rotated vehicle boxes"
    );

    crossing.center = {5.0f, 0.0f, 0.0f};

    require(
        !vehicleOrientedBoxesOverlap(
            parked,
            crossing
        ),
        "SAT rejects separated rotated vehicle boxes"
    );

    std::vector<VehicleOrientedBox> spatialFleet = {
        parked,
        crossing
    };

    const auto occupiedPoint =
        evaluatePointAgainstFleet(
            {0.0f, 0.0f, 0.0f},
            spatialFleet,
            true
        );

    require(
        occupiedPoint.verified &&
        occupiedPoint.insideAnyVehicle &&
        occupiedPoint.containingVehicle == parked.identity,
        "complete fleet verifies point occupancy"
    );

    VehicleOrientedBox candidateFootprint = parked;
    candidateFootprint.identity = 0;
    candidateFootprint.center = {9.0f, 0.0f, 0.0f};

    const auto clearFleet =
        evaluateFootprintAgainstFleet(
            candidateFootprint,
            spatialFleet,
            true
        );

    require(
        clearFleet.verified &&
        !clearFleet.overlaps,
        "complete fleet can verify clear candidate footprint"
    );

    candidateFootprint.center = {0.5f, 0.0f, 0.0f};

    const auto overlappingFleet =
        evaluateFootprintAgainstFleet(
            candidateFootprint,
            spatialFleet,
            true
        );

    require(
        overlappingFleet.verified &&
        overlappingFleet.overlaps &&
        overlappingFleet.overlappingVehicle ==
            parked.identity,
        "complete fleet identifies overlapping live vehicle"
    );

    VehicleOrientedBox unreadable{};
    unreadable.identity = 0x4444;
    spatialFleet.push_back(unreadable);

    const auto incompleteEvidence =
        evaluateFootprintAgainstFleet(
            candidateFootprint,
            spatialFleet,
            true
        );

    require(
        !incompleteEvidence.verified &&
        incompleteEvidence.invalidVehicles == 1,
        "one unreadable live vehicle fails overlap evidence closed"
    );

    VehicleFootprintTuning footprintTuning{};
    footprintTuning.minimumSamples = 4;
    footprintTuning.maximumRelativeSpread = 0.03f;

    VehicleFootprintLearner footprintLearner(
        footprintTuning
    );

    for (int i = 0; i < 3; ++i) {
        VehicleOrientedBox sample = parked;
        sample.halfExtents = {
            1.0f + 0.005f * static_cast<float>(i),
            0.75f,
            2.1f
        };
        footprintLearner.observe(sample);
    }

    auto learnedFootprint =
        footprintLearner.estimate(
            parked.vehicleKey
        );

    require(
        learnedFootprint.found &&
        !learnedFootprint.verified &&
        learnedFootprint.sampleCount == 3,
        "footprint model exists but remains unverified before minimum samples"
    );

    VehicleOrientedBox fourthSample = parked;
    fourthSample.halfExtents = {
        1.01f,
        0.75f,
        2.1f
    };
    footprintLearner.observe(fourthSample);

    learnedFootprint =
        footprintLearner.estimate(
            parked.vehicleKey
        );

    require(
        learnedFootprint.verified &&
        learnedFootprint.sampleCount == 4 &&
        learnedFootprint.maximumObservedRelativeSpread < 0.03f,
        "four consistent rigid-body samples verify a learned model footprint"
    );

    require(
        footprintLearner.verifiedModelCount() == 1,
        "learner reports verified model coverage"
    );

    const auto roadAlignedFootprint =
        makeRoadAlignedVehicleFootprint(
            parked.vehicleKey,
            {10.0f, 0.0f, 20.0f},
            {0.0f, 0.0f, 5.0f},
            learnedFootprint.meanHalfExtents
        );

    require(
        roadAlignedFootprint.valid &&
        roadAlignedFootprint.vehicleKey ==
            parked.vehicleKey &&
        roadAlignedFootprint.center.x == 10.0f &&
        roadAlignedFootprint.right.x > 0.99f &&
        roadAlignedFootprint.up.y > 0.99f &&
        roadAlignedFootprint.forward.z > 0.99f,
        "verified footprint becomes a road-aligned pre-construction OBB"
    );

    std::vector<VehicleOrientedBox> distantFleet = {
        parked
    };

    const auto learnedOverlap =
        evaluateFootprintAgainstFleet(
            roadAlignedFootprint,
            distantFleet,
            true
        );

    require(
        learnedOverlap.verified &&
        !learnedOverlap.overlaps,
        "learned pre-construction footprint can prove a clear live-vehicle area"
    );

    VehicleFootprintLearner unstableFootprint(
        footprintTuning
    );

    for (int i = 0; i < 4; ++i) {
        VehicleOrientedBox sample = parked;
        sample.vehicleKey = 0x12345678u;
        sample.halfExtents = {
            i == 3 ? 1.5f : 1.0f,
            0.75f,
            2.1f
        };
        unstableFootprint.observe(sample);
    }

    const auto unstableEstimate =
        unstableFootprint.estimate(
            0x12345678u
        );

    require(
        unstableEstimate.found &&
        !unstableEstimate.verified &&
        unstableEstimate.maximumObservedRelativeSpread > 0.03f,
        "inconsistent dimensions fail learned footprint verification closed"
    );

    MotionScaleTuning motionTuning{};
    motionTuning.minimumStableSamples = 4;
    motionTuning.maximumCoefficientOfVariation = 0.01f;

    MotionScaleObserver motionObserver(motionTuning);

    MotionScaleFrame motionFrame{};
    motionFrame.valid = true;
    motionFrame.safeFreeRoam = true;
    motionFrame.grounded = true;
    motionFrame.engineSpeed = 10.0f;
    motionFrame.speedometer = 36.0f;
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
        motionSnapshot.meanSpeedometerToEngineSpeedRatio > 3.59f &&
        motionSnapshot.meanSpeedometerToEngineSpeedRatio < 3.61f &&
        motionSnapshot.meanAbsoluteToEngineSpeedRatio > 0.99f &&
        motionSnapshot.meanAbsoluteToEngineSpeedRatio < 1.01f,
        "motion observer exposes speedometer and absolute-speed ratios"
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
    fast.speedometer = 36.0f;
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
            MutationReadinessBlocker::ExactRoadCandidateUnavailable,
        "lookahead alone is insufficient without exact WRoadNav geometry"
    );

    readiness.exactRoadCandidateObserved = true;
    readinessReport = evaluateMutationReadiness(readiness);
    require(
        readinessReport.blocker ==
            MutationReadinessBlocker::VehicleSpatialEvidenceUnavailable,
        "readiness requires complete live-vehicle spatial evidence"
    );

    readiness.vehicleSpatialEvidenceObserved = true;
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

    frr::game::PlayerRoadNavigationProbe roadProbe{};
    roadProbe.available = true;
    roadProbe.playerPosition = {0.0f, 0.0f, 0.0f, true};

    roadProbe.current.available = true;
    roadProbe.current.valid = true;
    roadProbe.current.segmentIndex = 10;
    roadProbe.current.laneIndex = 1;
    roadProbe.current.position = {0.0f, 0.0f, 0.0f, true};
    roadProbe.current.forward = {1.0f, 0.0f, 0.0f, true};
    roadProbe.current.roadWidthWorldUnits = 16.0f;
    roadProbe.current.segmentSpanWorldUnits = 200.0f;
    roadProbe.current.curvature = 0.005f;

    roadProbe.future = roadProbe.current;
    roadProbe.future.segmentIndex = 11;
    roadProbe.future.position = {800.0f, 0.0f, 0.0f, true};

    roadProbe.seekAheadPosition = {400.0f, 0.0f, 0.0f, true};
    roadProbe.seekAheadDistanceWorldUnits = 400.0f;
    roadProbe.seekAheadProjectionWorldUnits = 400.0f;

    roadProbe.farFuturePosition = {900.0f, 0.0f, 0.0f, true};
    roadProbe.farFutureDistanceWorldUnits = 900.0f;
    roadProbe.farFutureProjectionWorldUnits = 900.0f;

    const auto observedRoadCandidates =
        frr::game::RoadCandidateProbe::build(roadProbe);

    require(
        observedRoadCandidates.size() == 4,
        "road probe exposes current, future, seek-ahead and far-future observations"
    );

    const auto& currentObservation =
        observedRoadCandidates[0];
    const auto& futureObservation =
        observedRoadCandidates[1];
    const auto& seekObservation =
        observedRoadCandidates[2];

    require(
        inspectRoadCandidate(currentObservation) ==
            RoadCandidateBlocker::NotAhead,
        "current road position is not promoted as an ahead candidate"
    );

    require(
        inspectRoadCandidate(futureObservation) ==
            RoadCandidateBlocker::None,
        "future WRoadNav position carries exact promotable road geometry"
    );

    require(
        inspectRoadCandidate(seekObservation) ==
            RoadCandidateBlocker::RoadGeometryAssociationUnverified,
        "SeekAhead stays observational until exact road association is proven"
    );

    RoadCandidateEvidence roadEvidence{};
    auto roadSpawn =
        promoteRoadCandidateForSpawn(
            futureObservation,
            roadEvidence,
            rawScale,
            true
        );

    require(
        !roadSpawn.promotable &&
        roadSpawn.blocker ==
            RoadCandidateBlocker::MetricCalibrationUnverified,
        "road candidate cannot promote through an unverified metric scale"
    );

    roadSpawn =
        promoteRoadCandidateForSpawn(
            futureObservation,
            roadEvidence,
            verifiedScale,
            true
        );

    require(
        !roadSpawn.promotable &&
        roadSpawn.blocker ==
            RoadCandidateBlocker::StreamingUnverified,
        "metric geometry still requires streaming evidence"
    );

    roadEvidence.streamingVerified = true;
    roadEvidence.groundVerified = true;
    roadEvidence.groundValid = true;
    roadEvidence.visibilityVerified = true;
    roadEvidence.visibleToPlayer = false;
    roadEvidence.overlapVerified = true;
    roadEvidence.overlapsLiveVehicle = false;

    roadSpawn =
        promoteRoadCandidateForSpawn(
            futureObservation,
            roadEvidence,
            verifiedScale,
            true
        );

    require(
        roadSpawn.promotable &&
        roadSpawn.metric.available &&
        roadSpawn.transform.available &&
        roadSpawn.transform.position.x == 800.0f &&
        roadSpawn.transform.forward.x == 1.0f &&
        roadSpawn.metric.distanceMeters == 400.0f &&
        roadSpawn.metric.roadWidthMeters == 8.0f,
        "fully evidenced future road promotes with metric input and exact transform"
    );

    SpawnEnvironmentInput promotedSpawnEnv{};
    promotedSpawnEnv.experimentalFeatureEnabled = true;
    promotedSpawnEnv.supportedExecutable = true;
    promotedSpawnEnv.freeRoamCandidate = true;
    promotedSpawnEnv.playerAvailable = true;
    promotedSpawnEnv.independentPlayerCrossCheck = true;
    promotedSpawnEnv.roadNetworkAvailable = true;
    promotedSpawnEnv.stableFreeRoamSamples = 6;
    promotedSpawnEnv.liveRivals = 0;
    promotedSpawnEnv.maxLiveRivals = 1;

    const auto promotedSpawnDecision =
        evaluateSpawnCandidate(
            promotedSpawnEnv,
            roadSpawn.candidate
        );

    require(
        promotedSpawnDecision.allowed,
        "promoted road candidate passes existing spawn safety thresholds"
    );

    roadEvidence.visibleToPlayer = true;
    roadSpawn =
        promoteRoadCandidateForSpawn(
            futureObservation,
            roadEvidence,
            verifiedScale,
            true
        );

    require(
        !roadSpawn.promotable &&
        roadSpawn.blocker ==
            RoadCandidateBlocker::VisibleToPlayer,
        "spawn promotion blocks a fully evidenced point that is on-screen"
    );

    // A staging destination may be visible because both cars can drive to it;
    // hidden/off-screen is a spawn pop-in rule, not a cinematic-site rule.
    roadEvidence.junctionVerified = true;
    roadEvidence.junction = false;
    roadEvidence.obstructionVerified = true;
    roadEvidence.obstructed = false;
    roadEvidence.gradeVerified = true;
    roadEvidence.absoluteGrade = 0.02f;
    roadEvidence.twoCarGeometryVerified = true;
    roadEvidence.supportsTwoCars = true;

    RoadCandidateObservation stagingObservation =
        futureObservation;
    stagingObservation.distanceWorldUnits = 240.0f;
    stagingObservation.forwardProjectionWorldUnits = 240.0f;
    stagingObservation.position.x = 240.0f;

    const auto roadStaging =
        promoteRoadCandidateForStaging(
            stagingObservation,
            roadEvidence,
            verifiedScale
        );

    require(
        roadStaging.promotable &&
        roadStaging.transform.available &&
        roadStaging.transform.position.x == 240.0f &&
        roadStaging.candidate.metricGeometryVerified &&
        roadStaging.candidate.distanceAheadMeters == 120.0f &&
        scoreStagingCandidate(
            roadStaging.candidate
        ).eligible,
        "visible but otherwise safe 120 m road candidate keeps transform and enters staging scorer"
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
