#include "RuntimeProbe.h"

#include "ChallengeInputProbe.h"
#include "GameplayLoopHook.h"
#include "CameraFrustumProbe.h"
#include "GameBridge.h"
#include "RoadCandidateProbe.h"
#include "RenderObservationHook.h"
#include "VehicleCatalogProbe.h"
#include "VehicleSpatialProbe.h"
#include "WorldCollisionProbe.h"
#include "../core/Log.h"
#include "../domain/MotionScaleObserver.h"
#include "../domain/MutationReadiness.h"
#include "../domain/RoadCandidatePlanner.h"
#include "../domain/RuntimeSession.h"
#include "../domain/RuntimeEvidence.h"
#include "../domain/SelectedRivalEvidence.h"
#include "../domain/RivalPopulation.h"
#include "../domain/SpawnSafety.h"
#include "../domain/UndergroundBlacklist.h"
#include "../domain/VehicleFootprintLearning.h"
#include "../domain/VehicleSelection.h"
#include "../domain/VehicleSpatialEvidence.h"
#include "../domain/WorldCollisionEvidence.h"
#include "../persistence/UndergroundBlacklistStore.h"


#include <windows.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace frr::game {
namespace {

RuntimeProbeConfig g_config{};
RuntimeSnapshot g_last{};
bool g_haveLast = false;

std::atomic<std::uint64_t> g_renderFrames{0};
std::atomic<std::uint64_t> g_inputPolls{0}; // legacy native poller is not installed
std::atomic<std::uint64_t> g_gameplayCallbacks{0};
std::atomic<std::uint64_t> g_frameTicks{0};
std::atomic<std::uint64_t> g_samples{0};

std::atomic<DWORD> g_renderThreadId{0};
std::atomic<DWORD> g_inputThreadId{0};
std::atomic<DWORD> g_frameTickThreadId{0};

std::atomic<bool> g_frameTickProbeInstalled{false};
std::atomic<bool> g_safeFreeRoamObserved{false};
std::atomic<bool> g_roadLookaheadObserved{false};
std::atomic<bool> g_exactRoadCandidateObserved{false};
std::atomic<bool> g_vehicleSpatialEvidenceObserved{false};
std::atomic<bool> g_vehicleFootprintVerified{false};
std::atomic<bool> g_groundEvidenceObserved{false};
std::atomic<bool> g_worldOcclusionEvidenceObserved{false};
std::atomic<std::uint32_t> g_verifiedFootprintVehicleKey{0};

frr::domain::VehicleFootprintLearner g_vehicleFootprintLearner{};

struct WorldCollisionRequest {
    bool pending = false;
    std::uint64_t id = 0;
    frr::domain::RuntimeEvidenceStamp stamp{};
    frr::domain::SpatialVector3 candidate{};
    frr::domain::SpatialVector3 lineOrigin{};
    frr::domain::SpatialVector3 lineTarget{};
    bool lineAvailable = false;
};

struct WorldCollisionResult {
    bool available = false;
    std::uint64_t id = 0;
    frr::domain::RuntimeEvidenceStamp stamp{};
    frr::domain::SpatialVector3 candidate{};
    frr::domain::GroundEvidence ground{};
    frr::domain::WorldOcclusionEvidence occlusion{};
};

SRWLOCK g_worldCollisionLock = SRWLOCK_INIT;
WorldCollisionRequest g_worldCollisionRequest{};
WorldCollisionResult g_worldCollisionResult{};
frr::domain::RuntimeEvidenceStamp g_latestEvidenceStamp{};
std::optional<frr::domain::GeneratedRival> g_selectedRival;
std::uint64_t g_selectedRivalProfile = 0;
frr::domain::SelectedRivalVehicle g_selectedRivalVehicle{};
std::uint64_t g_worldCollisionNextRequestId = 1;

bool g_vehicleCatalogValidated = false;

bool g_haveBlacklistDiagnostic = false;
bool g_lastBlacklistUnlocked = false;
bool g_lastBlacklistCompleted = false;
int g_lastBlacklistRank = -1;

bool g_blacklistProfileBound = false;
bool g_blacklistFileExists = false;
bool g_blacklistPersistenceHealthy = true;
std::uint64_t g_blacklistProfileKey = 0;
frr::domain::UndergroundBlacklistProgress g_blacklistProgress{};

frr::domain::RuntimeSessionTracker g_runtimeSession{};

frr::domain::MotionScaleObserver g_motionScaleObserver{};
frr::domain::MotionScaleSnapshot g_motionScaleSnapshot{};
bool g_haveMotionClock = false;
std::chrono::steady_clock::time_point g_lastMotionClock{};
bool g_loggedStableMotionScale = false;
frr::domain::RuntimeEvidenceStamp g_motionEvidenceStamp{};
std::uint32_t g_motionVehicleKey = 0;
std::uint64_t g_motionCaptureId = 0;
std::uint64_t g_motionCohortId = 0;

bool g_haveSpawnPreflight = false;
frr::domain::SpawnRejectReason g_lastSpawnPreflightReason =
    frr::domain::SpawnRejectReason::ExperimentalFeatureDisabled;

frr::domain::RuntimeEvidenceStamp evidenceStamp(
    const RuntimeSnapshot& current,
    std::uint64_t generation
) {
    frr::domain::RuntimeEvidenceStamp stamp{};
    stamp.safeFreeRoam =
        current.mode == WorldProbeMode::FreeRoamCandidate &&
        current.capabilities.canClassifyFreeRoam &&
        current.capabilities.roadNetworkAvailable &&
        current.vehicles.independentPlayerCrossCheck &&
        !current.raceStatusLoading && !current.inNIS && !current.fadeScreen;
    stamp.generation = generation;
    stamp.playerIVehicle = current.vehicles.playerIVehicle;
    stamp.playerPVehicle = current.vehicles.playerPVehicle;
    stamp.roadNetwork = current.roadNetwork;
    stamp.raceStatus = current.raceStatus;
    stamp.profileKey = current.career.profileKeyAvailable ? current.career.profileKey : 0;
    stamp.capturedAtMillis = GetTickCount64();
    return stamp;
}

void selectPendingRival(const RuntimeSnapshot& current) {
    g_selectedRivalVehicle = {};
    if (!current.career.available || !current.career.profileKeyAvailable ||
        current.career.profileKey == 0) {
        g_selectedRival.reset();
        g_selectedRivalProfile = 0;
        return;
    }
    if (!g_selectedRival || g_selectedRivalProfile != current.career.profileKey) {
        frr::domain::ProceduralRivalRequest request{};
        request.seed = current.career.profileKey ^ 0x4652525331ull;
        // Career completion is verified, partial career progress is not yet
        // read. Keep the initial experiment at Tier 1 before completion.
        request.maximumTier = current.career.careerCompletedAtLeastOnce ? 5 : 1;
        request.rockportLegend = current.career.careerCompletedAtLeastOnce;
        g_selectedRival = frr::domain::generateProceduralRival(request);
        g_selectedRivalProfile = current.career.profileKey;
        if (g_selectedRival) {
            std::ostringstream line;
            line << "Pending first-spawn rival selected: rivalId=" << g_selectedRival->rivalId
                 << " name=" << g_selectedRival->name
                 << " vehicle=" << g_selectedRival->vehicleKey
                 << ". Selection is retained; missing footprint never rerolls the vehicle. No construction enabled.";
            Log::instance().info(line.str());
        }
    }
    if (!g_selectedRival) return;
    const auto key = VehicleCatalogProbe::runtimeKeyForName(g_selectedRival->vehicleKey);
    g_selectedRivalVehicle = {g_selectedRival->rivalId, key.value_or(0)};
}

bool gameplayLoopThreadConfirmed() {
    const auto loop = GameplayLoopHook::snapshot();
    const auto frameThread = g_frameTickThreadId.load();
    return g_config.frameTickProbeEnabled && g_frameTickProbeInstalled.load() &&
        loop.installed && loop.sourceVerified && loop.threadConsistent && loop.completed > 0 &&
        frameThread != 0 && frameThread == loop.threadId;
}
bool collisionGameplayThreadConfirmed() {
    return g_config.worldCollisionDiagnosticsEnabled && gameplayLoopThreadConfirmed() &&
        GetCurrentThreadId() == GameplayLoopHook::snapshot().threadId;
}

void queueWorldCollisionRequest(
    const std::vector<
        frr::domain::RoadCandidateObservation
    >& roadCandidates,
    const RuntimeSnapshot& current
) {
    if (!g_config.worldCollisionDiagnosticsEnabled ||
        !g_config.frameTickProbeEnabled ||
        current.mode != WorldProbeMode::FreeRoamCandidate ||
        !current.capabilities.canClassifyFreeRoam) {
        return;
    }

    const frr::domain::RoadCandidateObservation*
        selected = nullptr;

    for (const auto& candidate : roadCandidates) {
        if (frr::domain::inspectRoadCandidate(candidate) ==
            frr::domain::RoadCandidateBlocker::None &&
            candidate.position.finite) {
            selected = &candidate;
            break;
        }
    }

    if (!selected) {
        return;
    }

    WorldCollisionRequest request{};
    request.stamp = evidenceStamp(current, g_runtimeSession.snapshot().generation);
    request.candidate = {
        selected->position.x,
        selected->position.y,
        selected->position.z
    };

    if (current.playerMotion.position.finite) {
        request.lineAvailable = true;
        request.lineOrigin = {
            current.playerMotion.position.x,
            current.playerMotion.position.y,
            current.playerMotion.position.z
        };

        request.lineTarget = request.candidate;

        const std::uint32_t footprintKey =
            g_verifiedFootprintVehicleKey.load(
                std::memory_order_relaxed
            );

        if (footprintKey != 0) {
            const auto estimate =
                g_vehicleFootprintLearner.estimate(
                    footprintKey
                );

            if (estimate.verified) {
                request.lineTarget.y +=
                    estimate.maximumHalfExtents.y;
            }
        }
    }

    AcquireSRWLockExclusive(
        &g_worldCollisionLock
    );

    if (frr::domain::runtimeEvidenceUsable(request.stamp, g_latestEvidenceStamp, GetTickCount64())) {
        request.pending = true;
        request.id =
            g_worldCollisionNextRequestId++;
        g_worldCollisionRequest = request;
    }

    ReleaseSRWLockExclusive(
        &g_worldCollisionLock
    );
}

void processWorldCollisionRequest() {
    if (!collisionGameplayThreadConfirmed()) {
        return;
    }

    WorldCollisionRequest request{};

    AcquireSRWLockExclusive(
        &g_worldCollisionLock
    );

    if (g_worldCollisionRequest.pending) {
        request = g_worldCollisionRequest;
        g_worldCollisionRequest.pending = false;
    }

    ReleaseSRWLockExclusive(
        &g_worldCollisionLock
    );

    if (request.id == 0) {
        return;
    }

    // Revalidate live world identity on the confirmed gameplay thread. A
    // render-side observation cannot authorize a query after a transition.
    frr::domain::RuntimeEvidenceStamp latest{};
    AcquireSRWLockShared(&g_worldCollisionLock);
    latest = g_latestEvidenceStamp;
    ReleaseSRWLockShared(&g_worldCollisionLock);
    const auto live = evidenceStamp(GameBridge::sample(), latest.generation);
    if (!frr::domain::runtimeEvidenceUsable(request.stamp, latest, GetTickCount64()) ||
        !frr::domain::runtimeEvidenceUsable(request.stamp, live, GetTickCount64())) {
        AcquireSRWLockExclusive(&g_worldCollisionLock);
        g_worldCollisionResult = {};
        ReleaseSRWLockExclusive(&g_worldCollisionLock);
        g_groundEvidenceObserved.store(false, std::memory_order_relaxed);
        g_worldOcclusionEvidenceObserved.store(false, std::memory_order_relaxed);
        return;
    }

    const auto groundSample =
        WorldCollisionProbe::sampleGround(
            request.candidate
        );

    WorldCollisionResult result{};
    result.available = true;
    result.id = request.id;
    result.stamp = request.stamp;
    result.candidate = request.candidate;
    result.ground =
        frr::domain::interpretGroundCollision(
            request.candidate,
            groundSample
        );

    if (request.lineAvailable) {
        const auto occlusionSample =
            WorldCollisionProbe::
                sampleWorldOcclusion(
                    request.lineOrigin,
                    request.lineTarget
                );

        result.occlusion =
            frr::domain::interpretWorldOcclusion(
                occlusionSample
            );
    }

    AcquireSRWLockExclusive(&g_worldCollisionLock);
    if (frr::domain::runtimeEvidenceUsable(result.stamp, g_latestEvidenceStamp, GetTickCount64())) {
        g_worldCollisionResult = result;
    }
    ReleaseSRWLockExclusive(&g_worldCollisionLock);

}

WorldCollisionResult latestWorldCollisionResult() {
    WorldCollisionResult result{};

    AcquireSRWLockShared(
        &g_worldCollisionLock
    );
    if (frr::domain::runtimeEvidenceUsable(
            g_worldCollisionResult.stamp, g_latestEvidenceStamp, GetTickCount64())) {
        result = g_worldCollisionResult;
    }
    ReleaseSRWLockShared(
        &g_worldCollisionLock
    );

    return result;
}

bool sameMeaningfulState(
    const RuntimeSnapshot& a,
    const RuntimeSnapshot& b
) {
    return
        a.inWorld == b.inWorld &&
        a.inNIS == b.inNIS &&
        a.fadeScreen == b.fadeScreen &&
        a.gameFlowState == b.gameFlowState &&
        a.raceStatus == b.raceStatus &&
        a.raceStatusLoading == b.raceStatusLoading &&
        a.racePlayMode == b.racePlayMode &&
        a.roadNetwork == b.roadNetwork &&
        a.vehicles.registryReadable ==
            b.vehicles.registryReadable &&
        a.vehicles.playerIVehicle ==
            b.vehicles.playerIVehicle &&
        a.vehicles.playerPVehicle ==
            b.vehicles.playerPVehicle &&
        a.vehicles.pvehicleRegistryCount ==
            b.vehicles.pvehicleRegistryCount &&
        a.vehicles.independentPlayerCrossCheck ==
            b.vehicles.independentPlayerCrossCheck &&
        a.vehicles.totalVehicles ==
            b.vehicles.totalVehicles &&
        a.vehicles.humanVehicles ==
            b.vehicles.humanVehicles &&
        a.vehicles.trafficVehicles ==
            b.vehicles.trafficVehicles &&
        a.vehicles.copVehicles ==
            b.vehicles.copVehicles &&
        a.vehicles.racerVehicles ==
            b.vehicles.racerVehicles &&
        a.playerMotion.available ==
            b.playerMotion.available &&
        a.roadNavigation.available ==
            b.roadNavigation.available &&
        a.roadNavigation.playerAiAvailable ==
            b.roadNavigation.playerAiAvailable &&
        a.career.available == b.career.available &&
        a.career.cash == b.career.cash &&
        a.career.careerCars == b.career.careerCars &&
        a.career.currentCarHandle ==
            b.career.currentCarHandle &&
        a.career.careerCompletedAtLeastOnce ==
            b.career.careerCompletedAtLeastOnce &&
        a.career.profileKeyAvailable ==
            b.career.profileKeyAvailable &&
        a.career.profileKey ==
            b.career.profileKey &&
        a.mode == b.mode;
}

std::string describe(const RuntimeSnapshot& s) {
    std::ostringstream out;

    out << "mode=" << worldProbeModeName(s.mode)
        << " inWorld=" << (s.inWorld ? 1 : 0)
        << " raceMode=" << racePlayModeName(s.racePlayMode)
        << " loading=" << (s.raceStatusLoading ? 1 : 0)
        << " inNIS=" << (s.inNIS ? 1 : 0)
        << " fade=" << (s.fadeScreen ? 1 : 0)
        << " flow=" << s.gameFlowState
        << " IVehicle=0x"
        << std::hex << std::uppercase
        << s.vehicles.playerIVehicle
        << " PVehicle=0x"
        << s.vehicles.playerPVehicle
        << " raceStatus=0x"
        << s.raceStatus
        << " roadNetwork=0x"
        << s.roadNetwork
        << std::dec
        << " pvehicleCount="
        << s.vehicles.pvehicleRegistryCount
        << " playerCrossCheck="
        << (s.vehicles.independentPlayerCrossCheck ? 1 : 0)
        << " vehicles=" << s.vehicles.totalVehicles
        << " human=" << s.vehicles.humanVehicles
        << " traffic=" << s.vehicles.trafficVehicles
        << " cops=" << s.vehicles.copVehicles
        << " racers=" << s.vehicles.racerVehicles
        << " none=" << s.vehicles.noneVehicles
        << " nisCars=" << s.vehicles.nisVehicles
        << " remote=" << s.vehicles.remoteVehicles
        << " unknown=" << s.vehicles.unknownVehicles;

    if (s.career.available) {
        out << " cash=" << s.career.cash
            << " careerCars=" << s.career.careerCars
            << " currentCarHandle="
            << s.career.currentCarHandle
            << " careerCompleted="
            << (s.career.careerCompletedAtLeastOnce ? 1 : 0)
            << " profileKey="
            << (s.career.profileKeyAvailable
                ? "pseudonymous"
                : "unavailable");
    } else {
        out << " career=unavailable";
    }

    out << " caps=[world:"
        << (s.capabilities.canObserveWorld ? 1 : 0)
        << ",player:"
        << (s.capabilities.canIdentifyPlayer ? 1 : 0)
        << ",freeroam:"
        << (s.capabilities.canClassifyFreeRoam ? 1 : 0)
        << ",road:"
        << (s.capabilities.roadNetworkAvailable ? 1 : 0)
        << ",roadNav:"
        << (s.capabilities.roadNavigationReadAvailable ? 1 : 0)
        << ",careerRead:"
        << (s.capabilities.careerReadAvailable ? 1 : 0)
        << ",spawnWrite:"
        << (s.capabilities.rivalSpawnExperimentVerified ? 1 : 0)
        << ",economyWrite:"
        << (s.capabilities.economyWriteVerified ? 1 : 0)
        << ",garageWrite:"
        << (s.capabilities.garageWriteVerified ? 1 : 0)
        << "]";

    if (s.playerMotion.available) {
        out << " motion=[speed:"
            << std::fixed << std::setprecision(3)
            << s.playerMotion.speed
            << ",speedometer:"
            << s.playerMotion.speedometer
            << ",absSpeed:"
            << s.playerMotion.absoluteSpeed
            << ",localMag:"
            << s.playerMotion.localVelocityMagnitude
            << ",linearMag:"
            << s.playerMotion.linearVelocityMagnitude
            << ",slip:"
            << s.playerMotion.slipAngle
            << ",wheels:"
            << s.playerMotion.wheelsOnGround
            << ",units:engine/unverified]";
    } else {
        out << " motion=unavailable";
    }

    if (g_config.roadNavDiagnosticsEnabled) {
        if (s.roadNavigation.available) {
            out << " roadNav=[ai:0x"
                << std::hex << std::uppercase
                << s.roadNavigation.playerAi
                << std::dec;

            if (s.roadNavigation.current.available) {
                out << ",curSeg:"
                    << s.roadNavigation.current.segmentIndex
                    << ",curLane:"
                    << s.roadNavigation.current.laneIndex
                    << ",curWidthWorld:"
                    << std::fixed << std::setprecision(1)
                    << s.roadNavigation.current.roadWidthWorldUnits
                    << ",curSpanWorld:"
                    << s.roadNavigation.current.segmentSpanWorldUnits
                    << ",curSegTime:"
                    << std::setprecision(3)
                    << s.roadNavigation.current.segmentTime
                    << ",curCurve:"
                    << std::setprecision(4)
                    << s.roadNavigation.current.curvature
                    << ",curRoadOcc:"
                    << s.roadNavigation.current.roadOcclusion
                    << ",curAvoidOcc:"
                    << s.roadNavigation.current.avoidableOcclusion
                    << ",curOccBehind:"
                    << (s.roadNavigation.current.occludedFromBehind ? 1 : 0);
            } else {
                out << ",cur:unavailable";
            }

            if (s.roadNavigation.future.available) {
                out << ",futureSeg:"
                    << s.roadNavigation.future.segmentIndex
                    << ",futureLane:"
                    << s.roadNavigation.future.laneIndex
                    << ",futureWidthWorld:"
                    << std::fixed << std::setprecision(1)
                    << s.roadNavigation.future.roadWidthWorldUnits
                    << ",navGapWorld:"
                    << s.roadNavigation.currentToFutureWorldUnits
                    << ",futureRoadOcc:"
                    << s.roadNavigation.future.roadOcclusion
                    << ",futureAvoidOcc:"
                    << s.roadNavigation.future.avoidableOcclusion;
            } else {
                out << ",future:unavailable";
            }

            out << ",seekDistWorld:"
                << std::fixed << std::setprecision(1)
                << s.roadNavigation.seekAheadDistanceWorldUnits
                << ",seekProjWorld:"
                << s.roadNavigation.seekAheadProjectionWorldUnits
                << ",farDistWorld:"
                << s.roadNavigation.farFutureDistanceWorldUnits
                << ",farProjWorld:"
                << s.roadNavigation.farFutureProjectionWorldUnits;

            if (s.roadNavigation.playerPosition.finite) {
                out << ",playerPos:("
                    << s.roadNavigation.playerPosition.x << ","
                    << s.roadNavigation.playerPosition.y << ","
                    << s.roadNavigation.playerPosition.z << ")";
            }

            if (s.roadNavigation.seekAheadPosition.finite) {
                out << ",seek:("
                    << s.roadNavigation.seekAheadPosition.x << ","
                    << s.roadNavigation.seekAheadPosition.y << ","
                    << s.roadNavigation.seekAheadPosition.z << ")";
            }

            if (s.roadNavigation.farFuturePosition.finite) {
                out << ",far:("
                    << s.roadNavigation.farFuturePosition.x << ","
                    << s.roadNavigation.farFuturePosition.y << ","
                    << s.roadNavigation.farFuturePosition.z << ")";
            }

            out << ",units:world]";
        } else {
            out << " roadNav=unavailable";
        }
    }

    return out.str();
}

void updateRuntimeSessionAndSpawnPreflight(
    const RuntimeSnapshot& current
) {
    frr::domain::RuntimeSessionObservation observation{};
    observation.safeFreeRoam =
        current.mode == WorldProbeMode::FreeRoamCandidate &&
        current.capabilities.canClassifyFreeRoam &&
        current.capabilities.roadNetworkAvailable;
    observation.playerIdentity =
        current.vehicles.playerIVehicle;
    observation.roadNetworkIdentity =
        current.roadNetwork;

    const auto session = g_runtimeSession.tick(observation);
    const auto stamp = evidenceStamp(current, session.generation);
    AcquireSRWLockExclusive(&g_worldCollisionLock);
    g_latestEvidenceStamp = stamp;
    if (!session.active || session.newGeneration ||
        !frr::domain::runtimeEvidenceUsable(g_worldCollisionResult.stamp, stamp, GetTickCount64())) {
        g_worldCollisionResult = {};
    }
    if (!session.active || session.newGeneration) g_worldCollisionRequest = {};
    ReleaseSRWLockExclusive(&g_worldCollisionLock);
    if (!session.active || session.newGeneration) {
        g_vehicleFootprintLearner.reset();
        g_motionScaleObserver.reset();
        g_motionScaleSnapshot = {};
        g_haveMotionClock = false;
        g_loggedStableMotionScale = false;
        g_groundEvidenceObserved.store(false, std::memory_order_relaxed);
        g_worldOcclusionEvidenceObserved.store(false, std::memory_order_relaxed);
    }

    if (session.newGeneration) {
        std::ostringstream line;
        line << "Free Roam world generation "
             << session.generation
             << " started; runtime rival handles from older generations are invalid.";
        Log::instance().info(line.str());
    }

    if (!g_config.experimentalSpawnEnabled) {
        return;
    }

    frr::domain::SpawnEnvironmentInput environment{};
    environment.experimentalFeatureEnabled = true;
    environment.supportedExecutable = true;
    environment.freeRoamCandidate =
        current.mode == WorldProbeMode::FreeRoamCandidate;
    environment.loading = current.raceStatusLoading;
    environment.inNIS = current.inNIS;
    environment.fade = current.fadeScreen;
    environment.playerAvailable =
        current.vehicles.playerIVehicle != 0;
    environment.independentPlayerCrossCheck =
        current.vehicles.independentPlayerCrossCheck;
    environment.roadNetworkAvailable =
        current.roadNetwork != 0;
    environment.stableFreeRoamSamples =
        session.stableSamples;
    environment.liveRivals =
        static_cast<int>(current.vehicles.racerVehicles);
    environment.maxLiveRivals =
        g_config.maxActiveRivals;

    frr::domain::SpawnSafetyTuning tuning{};
    tuning.requiredStableSamples =
        g_config.stableFreeRoamSamplesBeforeSpawn;

    const auto decision =
        frr::domain::evaluateSpawnEnvironment(
            environment,
            tuning
        );

    if (!g_haveSpawnPreflight ||
        decision.reason != g_lastSpawnPreflightReason) {
        std::ostringstream line;
        line << "Experimental spawn preflight: ";

        if (decision.allowed) {
            line << "READY"
                 << " generation=" << session.generation
                 << " stableSamples=" << session.stableSamples
                 << ". Live road-nav telemetry is available separately; vehicle construction remains disabled in this build.";
            Log::instance().info(line.str());
        } else {
            line << "BLOCKED reason="
                 << frr::domain::spawnRejectReasonName(decision.reason)
                 << " generation=" << session.generation
                 << " stableSamples=" << session.stableSamples;
            Log::instance().warn(line.str());
        }

        g_haveSpawnPreflight = true;
        g_lastSpawnPreflightReason = decision.reason;
    }
}

frr::persistence::UndergroundBlacklistStore&
blacklistStore() {
    static frr::persistence::UndergroundBlacklistStore store(
        frr::persistence::UndergroundBlacklistStore::
            defaultSaveDirectory()
    );
    return store;
}

void resetBlacklistProfileBinding() {
    g_blacklistProfileBound = false;
    g_blacklistFileExists = false;
    g_blacklistPersistenceHealthy = true;
    g_blacklistProfileKey = 0;
    g_blacklistProgress = {};
    g_haveBlacklistDiagnostic = false;
}

void bindBlacklistProfile(
    const RuntimeSnapshot& current
) {
    if (!g_config.undergroundBlacklistPersistence ||
        !current.career.profileKeyAvailable) {
        if (g_blacklistProfileBound) {
            resetBlacklistProfileBinding();
        }
        return;
    }

    const std::uint64_t key =
        current.career.profileKey;

    if (g_blacklistProfileBound &&
        g_blacklistProfileKey == key) {
        return;
    }

    resetBlacklistProfileBinding();
    g_blacklistProfileBound = true;
    g_blacklistProfileKey = key;

    const auto loaded =
        blacklistStore().load(key);

    if (!loaded.ok) {
        g_blacklistPersistenceHealthy = false;
        Log::instance().warn(
            std::string(
                "Underground Blacklist persistence load failed: "
            ) + loaded.error
        );
        return;
    }

    if (loaded.found) {
        g_blacklistProgress = loaded.progress;
        g_blacklistFileExists = true;

        Log::instance().info(
            "Underground Blacklist mod-side progress loaded for current profile."
        );
    } else {
        Log::instance().info(
            "Underground Blacklist has no mod-side progress file for current profile yet."
        );
    }
}

void ensureBlacklistPersistenceFile(
    const RuntimeSnapshot& current
) {
    if (!g_config.undergroundBlacklistPersistence ||
        !g_blacklistProfileBound ||
        !g_blacklistPersistenceHealthy ||
        g_blacklistFileExists ||
        !current.career.careerCompletedAtLeastOnce) {
        return;
    }

    std::string error;
    if (blacklistStore().save(
            g_blacklistProfileKey,
            g_blacklistProgress,
            &error)) {
        g_blacklistFileExists = true;
        Log::instance().info(
            "Created mod-owned Underground Blacklist progress file after vanilla career completion."
        );
    } else {
        g_blacklistPersistenceHealthy = false;
        Log::instance().warn(
            std::string(
                "Could not create Underground Blacklist progress file: "
            ) + error
        );
    }
}

void updateUndergroundBlacklistDiagnostic(
    const RuntimeSnapshot& current
) {
    if (!g_config.undergroundBlacklistEnabled ||
        !current.career.available) {
        return;
    }

    bindBlacklistProfile(current);
    ensureBlacklistPersistenceFile(current);

    frr::domain::UndergroundBlacklistProgress progress =
        g_blacklistProfileBound
        ? g_blacklistProgress
        : frr::domain::UndergroundBlacklistProgress{};

    // Runtime facts are deliberately not loaded from the mod save.
    progress.careerCompleted =
        current.career.careerCompletedAtLeastOnce;
    progress.currentTargetPresent = false;

    const auto snapshot =
        frr::domain::evaluateUndergroundBlacklist(
            progress
        );

    const bool changed =
        !g_haveBlacklistDiagnostic ||
        snapshot.unlocked != g_lastBlacklistUnlocked ||
        snapshot.completed != g_lastBlacklistCompleted ||
        snapshot.currentRank != g_lastBlacklistRank;

    if (!changed) {
        return;
    }

    std::ostringstream line;
    line << "Underground Blacklist diagnostic: "
         << "unlocked=" << (snapshot.unlocked ? 1 : 0)
         << " completed=" << (snapshot.completed ? 1 : 0)
         << " currentRank=" << snapshot.currentRank
         << " targetSpawnEligible="
         << (snapshot.currentTargetSpawnEligible ? 1 : 0)
         << " persistence="
         << (g_config.undergroundBlacklistPersistence
             ? (g_blacklistPersistenceHealthy
                 ? (g_blacklistFileExists
                     ? "ready"
                     : "pending")
                 : "error")
             : "disabled")
         << ".";

    Log::instance().info(line.str());

    g_haveBlacklistDiagnostic = true;
    g_lastBlacklistUnlocked = snapshot.unlocked;
    g_lastBlacklistCompleted = snapshot.completed;
    g_lastBlacklistRank = snapshot.currentRank;
}

void sampleAndLog(
    std::uint64_t renderFrame
) {
    const RuntimeSnapshot current = GameBridge::sample();
    ++g_samples;
    updateRuntimeSessionAndSpawnPreflight(current);
    selectPendingRival(current);

    const auto motionStamp = evidenceStamp(current, g_runtimeSession.snapshot().generation);
    if (!frr::domain::sameRuntimeEvidenceContext(g_motionEvidenceStamp, motionStamp) ||
        current.playerMotion.vehicleKey == 0 || g_motionVehicleKey != current.playerMotion.vehicleKey) {
        g_motionScaleObserver.reset();
        g_motionScaleSnapshot = {};
        g_haveMotionClock = false;
        g_loggedStableMotionScale = false;
        ++g_motionCohortId;
    }
    g_motionEvidenceStamp = motionStamp;
    g_motionVehicleKey = current.playerMotion.vehicleKey;

    const auto motionNow =
        std::chrono::steady_clock::now();

    float motionDeltaSeconds = 0.0f;
    if (g_haveMotionClock) {
        motionDeltaSeconds =
            std::chrono::duration<float>(
                motionNow - g_lastMotionClock
            ).count();
    }

    g_lastMotionClock = motionNow;
    g_haveMotionClock = true;

    frr::domain::MotionScaleFrame motionFrame{};
    motionFrame.valid =
        current.playerMotion.available;
    motionFrame.safeFreeRoam =
        motionStamp.safeFreeRoam && g_motionVehicleKey != 0 &&
        frr::domain::sameRuntimeEvidenceContext(motionStamp, motionStamp);
    motionFrame.grounded =
        current.playerMotion.wheelsOnGround >= 3u;

    if (current.playerMotion.position.finite) {
        motionFrame.x = current.playerMotion.position.x;
        motionFrame.y = current.playerMotion.position.y;
        motionFrame.z = current.playerMotion.position.z;
    } else {
        motionFrame.valid = false;
    }

    motionFrame.engineSpeed =
        current.playerMotion.speed;
    motionFrame.speedometer =
        current.playerMotion.speedometer;
    motionFrame.absoluteSpeed =
        current.playerMotion.absoluteSpeed;
    motionFrame.localVelocityMagnitude =
        current.playerMotion.localVelocityMagnitude;
    motionFrame.linearVelocityMagnitude =
        current.playerMotion.linearVelocityMagnitude;

    g_motionScaleSnapshot =
        g_motionScaleObserver.push(
            motionFrame,
            motionDeltaSeconds
        );

    if (g_config.motionCaptureEnabled) {
        std::ostringstream line;
        line << std::setprecision(std::numeric_limits<float>::max_digits10)
             << "FRR_MOTION_V1 capture=" << g_motionCaptureId
             << " cohort=" << g_motionCohortId
             << " generation=" << motionStamp.generation
             << " model=" << g_motionVehicleKey
             << " timeMs=" << motionStamp.capturedAtMillis
             << " dt=" << motionDeltaSeconds
             << " valid=" << (motionFrame.valid ? 1 : 0)
             << " safe=" << (motionFrame.safeFreeRoam ? 1 : 0)
             << " grounded=" << (motionFrame.grounded ? 1 : 0)
             << " x=" << motionFrame.x << " y=" << motionFrame.y << " z=" << motionFrame.z
             << " speed=" << motionFrame.engineSpeed
             << " speedometer=" << motionFrame.speedometer
             << " absolute=" << motionFrame.absoluteSpeed
             << " local=" << motionFrame.localVelocityMagnitude
             << " linear=" << motionFrame.linearVelocityMagnitude
             << " vx=" << current.playerMotion.linearVelocity.x
             << " vy=" << current.playerMotion.linearVelocity.y
             << " vz=" << current.playerMotion.linearVelocity.z
             << " slip=" << current.playerMotion.slipAngle
             << " accepted=" << (g_motionScaleSnapshot.lastPairAccepted ? 1 : 0)
             << " window=" << g_motionScaleSnapshot.windowSamples
             << " ratio=" << g_motionScaleSnapshot.meanWorldUnitsPerSpeedUnitSecond
             << " cv=" << g_motionScaleSnapshot.coefficientOfVariation
             << " stable=" << (g_motionScaleSnapshot.stable ? 1 : 0)
             << " metricVerified=0";
        Log::instance().info(line.str());
    }

    if (g_motionScaleSnapshot.stable &&
        !g_loggedStableMotionScale) {
        std::ostringstream line;
        line << "Motion-scale observation became stable: "
             << "worldUnitsPerSpeedUnitSecond="
             << std::fixed << std::setprecision(5)
             << g_motionScaleSnapshot
                    .meanWorldUnitsPerSpeedUnitSecond
             << " cv="
             << g_motionScaleSnapshot
                    .coefficientOfVariation
             << " speedometerToSpeedRatio="
             << g_motionScaleSnapshot
                    .meanSpeedometerToEngineSpeedRatio
             << " absoluteToSpeedRatio="
             << g_motionScaleSnapshot
                    .meanAbsoluteToEngineSpeedRatio
             << " speedToLocalRatio="
             << g_motionScaleSnapshot
                    .meanSpeedToLocalVelocityRatio
             << " speedToLinearRatio="
             << g_motionScaleSnapshot
                    .meanSpeedToLinearVelocityRatio
             << ". This is consistency evidence only; metric calibration remains unverified.";

        Log::instance().info(line.str());
        g_loggedStableMotionScale = true;
    }

    const bool safeNow = g_runtimeSession.snapshot().active;
    g_safeFreeRoamObserved.store(safeNow, std::memory_order_relaxed);

    const auto roadCandidates =
        RoadCandidateProbe::build(
            current.roadNavigation
        );

    VehicleSpatialSnapshot vehicleSpatial{};
    if (safeNow && current.vehicles.registryReadable) {
        vehicleSpatial =
            VehicleSpatialProbe::sample();
    }

    g_roadLookaheadObserved.store(safeNow && current.roadNavigation.available, std::memory_order_relaxed);
    g_vehicleSpatialEvidenceObserved.store(
        safeNow && vehicleSpatial.registryComplete && vehicleSpatial.failedSpatialReads == 0,
        std::memory_order_relaxed);

    for (const auto& box : vehicleSpatial.boxes) {
        if (box.valid) {
            g_vehicleFootprintLearner.observe(box);
        }
    }

    const auto selectedEstimate = g_vehicleFootprintLearner.estimate(g_selectedRivalVehicle.vehicleKey);
    const bool selectedFootprintVerified = safeNow && g_selectedRivalVehicle.rivalId != 0 && selectedEstimate.verified;
    g_verifiedFootprintVehicleKey.store(
        selectedFootprintVerified ? g_selectedRivalVehicle.vehicleKey : 0, std::memory_order_relaxed);
    g_vehicleFootprintVerified.store(selectedFootprintVerified, std::memory_order_relaxed);

    bool exactRoadCandidateThisSample = false;
    for (const auto& candidate : roadCandidates) {
        if (frr::domain::inspectRoadCandidate(candidate) ==
            frr::domain::RoadCandidateBlocker::None) {
            exactRoadCandidateThisSample = true;
            break;
        }
    }

    g_exactRoadCandidateObserved.store(safeNow && exactRoadCandidateThisSample, std::memory_order_relaxed);
    queueWorldCollisionRequest(roadCandidates, current);
    const auto primaryCamera = safeNow && g_config.cameraFrustumDiagnosticsEnabled
        ? CameraFrustumProbe::sample() : frr::domain::PrimaryCameraSample{};
    const auto freshCollision = latestWorldCollisionResult();
    g_groundEvidenceObserved.store(
        freshCollision.available && freshCollision.ground.groundVerified && freshCollision.ground.groundValid,
        std::memory_order_relaxed);
    g_worldOcclusionEvidenceObserved.store(
        freshCollision.available && freshCollision.occlusion.occlusionVerified, std::memory_order_relaxed);

    updateUndergroundBlacklistDiagnostic(current);

    if (!g_vehicleCatalogValidated &&
        current.mode == WorldProbeMode::FreeRoamCandidate &&
        current.capabilities.canClassifyFreeRoam) {
        const auto catalog =
            VehicleCatalogProbe::validateDefaultCatalog();

        if (catalog.completed) {
            std::ostringstream catalogLine;
            catalogLine
                << "Vehicle catalog validation: "
                << catalog.available
                << "/"
                << catalog.configured
                << " configured pvehicle keys available.";

            if (!catalog.missingKeys.empty()) {
                catalogLine << " Missing:";
                for (const auto& key : catalog.missingKeys) {
                    catalogLine << " " << key;
                }
            }

            Log::instance().info(catalogLine.str());
            g_vehicleCatalogValidated = true;
        } else {
            Log::instance().warn(
                "Vehicle catalog validation could not complete; will retry on a later Free Roam sample."
            );
        }
    }

    const bool changed =
        !g_haveLast ||
        !sameMeaningfulState(current, g_last);

    const bool heartbeat =
        g_config.heartbeatFrames > 0 &&
        (renderFrame % g_config.heartbeatFrames) == 0;

    if (changed) {
        Log::instance().info(
            std::string("Runtime state changed: ") +
            describe(current)
        );
    } else if (heartbeat) {
        Log::instance().info(
            std::string("Runtime heartbeat: ") +
            describe(current)
        );
    }

    if (heartbeat &&
        current.inWorld &&
        current.vehicles.registryReadable) {
        std::ostringstream line;
        line << "Vehicle-spatial observation:"
             << " registryComplete="
             << (vehicleSpatial.registryComplete ? 1 : 0)
             << " registryCount="
             << vehicleSpatial.registryCount
             << " enabledActive="
             << vehicleSpatial.enabledActiveVehicles
             << " ignoredInactive="
             << vehicleSpatial.ignoredInactiveVehicles
             << " boxes="
             << vehicleSpatial.boxes.size()
             << " failedSpatialReads="
             << vehicleSpatial.failedSpatialReads
             << " halfExtentSemanticsResearchBacked="
             << (vehicleSpatial
                    .halfExtentSemanticsResearchBacked
                 ? 1
                 : 0)
             << " learnedModels="
             << g_vehicleFootprintLearner.modelCount()
             << " verifiedLearnedModels="
             << g_vehicleFootprintLearner.verifiedModelCount()
             << " catalogFootprintKey=0x"
             << std::hex << std::uppercase
             << g_verifiedFootprintVehicleKey.load(
                    std::memory_order_relaxed
                )
             << std::dec;

        Log::instance().info(line.str());
    }

    if (heartbeat && g_config.cameraFrustumDiagnosticsEnabled) {
        std::ostringstream line;
        line << "Primary-camera observation: coherent=" << (primaryCamera.available ? 1 : 0)
             << " viewId=" << primaryCamera.viewId
             << " coordinateMapping=sim(z,-x,y)_to_render"
             << " fullPlayerViewCoverageVerified=0 renderEnvelopeVerified=0 spawnVisibilityVerified=0";
        Log::instance().info(line.str());
    }
    if (heartbeat && !roadCandidates.empty()) {
        std::ostringstream line;
        line << "Road-candidate observations: count="
             << roadCandidates.size()
             << " exactUsable="
             << (exactRoadCandidateThisSample ? 1 : 0);

        for (const auto& candidate : roadCandidates) {
            const auto blocker =
                frr::domain::inspectRoadCandidate(
                    candidate
                );

            line << " ["
                 << frr::domain::roadCandidateSourceName(
                        candidate.source
                    )
                 << ":"
                 << frr::domain::roadCandidateBlockerName(
                        blocker
                    )
                 << ",seg=" << candidate.segmentIndex
                 << ",lane=" << candidate.laneIndex
                 << ",distWorld="
                 << std::fixed << std::setprecision(1)
                 << candidate.distanceWorldUnits
                 << ",projWorld="
                 << candidate.forwardProjectionWorldUnits;

            if (candidate.position.finite) {
                const auto occupancy =
                    frr::domain::evaluatePointAgainstFleet(
                        {
                            candidate.position.x,
                            candidate.position.y,
                            candidate.position.z
                        },
                        vehicleSpatial.boxes,
                        vehicleSpatial.registryComplete
                    );

                line << ",pointOccupancy="
                     << (occupancy.verified
                         ? (occupancy.insideAnyVehicle
                            ? "occupied"
                            : "clear")
                         : "unverified")
                     << ",fleetChecked="
                     << occupancy.checkedVehicles
                     << ",fleetInvalid="
                     << occupancy.invalidVehicles;

                if (occupancy.checkedVehicles > 0) {
                    line << ",nearestVehicleGapWorld="
                         << occupancy
                                .nearestSeparationWorldUnits;
                }

                const auto selectedOverlap = frr::domain::evaluateSelectedRivalOverlap(
                    g_selectedRivalVehicle, candidate, g_vehicleFootprintLearner,
                    vehicleSpatial.boxes, safeNow && vehicleSpatial.registryComplete);
                // Feed the actual selected model's SAT result into the existing
                // promotion contract. Unresolved evidence retains safe defaults.
                const auto promotion = frr::domain::promoteRoadCandidateForSpawn(
                    candidate, selectedOverlap.evidence, {}, g_selectedRivalVehicle.vehicleKey != 0);
                line << ",selectedRivalId=" << g_selectedRivalVehicle.rivalId
                     << ",selectedVehicleKey=0x" << std::hex << g_selectedRivalVehicle.vehicleKey << std::dec
                     << ",preconstructionOverlap="
                     << (selectedOverlap.evidence.overlapVerified
                         ? (selectedOverlap.evidence.overlapsLiveVehicle ? "occupied" : "clear") : "unverified")
                     << ",overlapVerified=" << (selectedOverlap.evidence.overlapVerified ? 1 : 0)
                     << ",promotion=" << frr::domain::roadCandidateBlockerName(promotion.blocker);
                if (g_config.cameraFrustumDiagnosticsEnabled) {
                    const auto cameraReport = frr::domain::classifyPrimaryCameraFootprint(primaryCamera, selectedOverlap.footprint);
                    line << ",primaryCamera=" << frr::domain::cameraBoxVisibilityName(cameraReport.visibility)
                         << ",separatingPlane=" << cameraReport.separatingPlane
                         << ",completeSpawnVisibilityVerified=0";
                }

            }

            line << "]";
        }

        Log::instance().info(line.str());
    }

    if (heartbeat &&
        g_config.worldCollisionDiagnosticsEnabled) {
        const auto collision =
            latestWorldCollisionResult();

        std::ostringstream line;
        line << "World-collision evidence:"
             << " addressAvailable="
             << (WorldCollisionProbe::addressAvailable()
                 ? 1
                 : 0)
             << " gameplayThreadConfirmed="
             << (gameplayLoopThreadConfirmed() ? 1 : 0);

        if (collision.available) {
            line << " requestId="
                 << collision.id
                 << " ground="
                 << (collision.ground.groundVerified
                     ? (collision.ground.groundValid
                        ? "valid"
                        : "invalid")
                     : "unverified")
                 << " groundDeltaWorld="
                 << std::fixed << std::setprecision(3)
                 << collision.ground
                        .absoluteHeightDeltaWorldUnits
                 << " grade="
                 << (collision.ground.gradeVerified
                     ? collision.ground.absoluteGrade
                     : -1.0f)
                 << " playerLineWorldOcclusion="
                 << (collision.occlusion.occlusionVerified
                     ? (collision.occlusion.occludedByWorld
                        ? "blocked"
                        : "clear")
                     : "unverified");
        } else {
            line << " result=pending_or_unavailable";
        }

        line << " cameraVisibilityVerified=0"
             << " streamingVerified=0";

        Log::instance().info(line.str());
    }

    if (heartbeat &&
        (g_motionScaleSnapshot.acceptedSamples > 0 ||
         g_motionScaleSnapshot.rejectedSamples > 0)) {
        std::ostringstream line;
        line << "Motion-scale observation: accepted="
             << g_motionScaleSnapshot.acceptedSamples
             << " rejected="
             << g_motionScaleSnapshot.rejectedSamples
             << " window=" << g_motionScaleSnapshot.windowSamples
             << " worldUnitsPerSpeedUnitSecond="
             << std::fixed << std::setprecision(5)
             << g_motionScaleSnapshot
                    .meanWorldUnitsPerSpeedUnitSecond
             << " cv="
             << g_motionScaleSnapshot
                    .coefficientOfVariation
             << " speedometerToSpeedRatio="
             << g_motionScaleSnapshot
                    .meanSpeedometerToEngineSpeedRatio
             << " absoluteToSpeedRatio="
             << g_motionScaleSnapshot
                    .meanAbsoluteToEngineSpeedRatio
             << " speedToLocalRatio="
             << g_motionScaleSnapshot
                    .meanSpeedToLocalVelocityRatio
             << " speedToLinearRatio="
             << g_motionScaleSnapshot
                    .meanSpeedToLinearVelocityRatio
             << " stable="
             << (g_motionScaleSnapshot.stable ? 1 : 0)
             << " metricVerified=0";

        Log::instance().info(line.str());
    }

    g_last = current;
    g_haveLast = true;
}

void rememberThread(
    std::atomic<DWORD>& slot
) {
    DWORD expected = 0;
    slot.compare_exchange_strong(
        expected,
        GetCurrentThreadId()
    );
}

void onGameplayLoopBefore(float) {
    if (g_config.frameTickProbeEnabled) {
        rememberThread(g_frameTickThreadId);
        ++g_frameTicks;
    }
}

void onRenderFrame(void*) {
    rememberThread(g_renderThreadId);
    const std::uint64_t frame = ++g_renderFrames;
    if (frame == 1) Log::instance().info("First render observation reached RuntimeProbe; runtime sampling is active.");

    if (g_config.sampleEveryFrames == 0 ||
        frame == 1 ||
        (frame % g_config.sampleEveryFrames) == 0) {
        sampleAndLog(frame);
    }
}

void onGameplayLoopAfter(float) {
    if (++g_gameplayCallbacks == 1)
        Log::instance().info("First verified gameplay-loop callback delivered after original update; native inputPolls remains separate.");
    if (g_config.inputProbeEnabled) {
        const auto before = ChallengeInputProbe::totalPresses();
        ChallengeInputProbe::onPoll();
        if (ChallengeInputProbe::totalPresses() != before)
            Log::instance().info("Fallback challenge key edge observed on gameplay loop (read-only; encounter dispatch not enabled).");
    }
    processWorldCollisionRequest();
}

DWORD WINAPI healthThread(LPVOID) {
    // We only inspect our own atomics here. No engine objects are
    // dereferenced from this background thread.
    const auto startedAt = GetTickCount64();
    Sleep(8000);
    for (;;) {
        const auto elapsedSeconds = (GetTickCount64() - startedAt) / 1000;
        const auto renderHook = RenderObservationHook::snapshot();
        const auto gameplayLoop = GameplayLoopHook::snapshot();

        std::ostringstream out;
        const DWORD renderThread = g_renderThreadId.load();
        const DWORD inputThread = g_inputThreadId.load();
        const DWORD frameThread = g_frameTickThreadId.load();

        out << "Runtime hook health after " << elapsedSeconds << "s:"
            << " renderFrames=" << g_renderFrames.load()
            << " inputPolls=" << g_inputPolls.load()
            << " gameplayLoopCalls=" << gameplayLoop.entered
            << " gameplayLoopCompleted=" << gameplayLoop.completed
            << " gameplayCallbacks=" << g_gameplayCallbacks.load()
            << " gameplayLoopInstalled=" << gameplayLoop.installed
            << " gameplayLoopSourceVerified=" << gameplayLoop.sourceVerified
            << " gameplayLoopThreadConsistent=" << gameplayLoop.threadConsistent
            << " gameplayLoopThread=" << gameplayLoop.threadId
            << " frameTicks=" << g_frameTicks.load()
            << " samples=" << g_samples.load()
            << " challengeFallbackPresses="
            << ChallengeInputProbe::totalPresses()
            << " renderThread=" << renderThread
            << " inputThread=" << inputThread
            << " frameTickThread=" << frameThread;
        out << " renderWorkerStarted=" << (renderHook.workerStarted ? 1 : 0)
            << " endSceneInstalled=" << (renderHook.endSceneInstalled ? 1 : 0)
            << " presentInstalled=" << (renderHook.presentInstalled ? 1 : 0)
            << " endSceneCalls=" << renderHook.endSceneCalls
            << " presentCalls=" << renderHook.presentCalls
            << " deviceGlobal=0x" << std::hex << renderHook.deviceGlobal
            << " device=0x" << renderHook.device << " vtable=0x" << renderHook.vtable << std::dec;

        if (frameThread != 0 && gameplayLoop.threadId != 0) {
            out << " frameTickEqualsGameplayLoop=" << (frameThread == gameplayLoop.threadId ? 1 : 0);
        }
        if (frameThread != 0 && inputThread != 0) {
            out << " frameTickEqualsInput="
                << (frameThread == inputThread ? 1 : 0);
        }

        if (frameThread != 0 && renderThread != 0) {
            out << " frameTickEqualsRender="
                << (frameThread == renderThread ? 1 : 0);
        }

        Log::instance().info(out.str());

        frr::domain::MutationReadinessInput readiness{};
        readiness.frameTickProbeEnabled =
            g_config.frameTickProbeEnabled;
        readiness.frameTickProbeInstalled =
            g_frameTickProbeInstalled.load(
                std::memory_order_relaxed
            );
        readiness.frameTickCount =
            g_frameTicks.load(
                std::memory_order_relaxed
            );
        readiness.inputPollCount =
            g_inputPolls.load(
                std::memory_order_relaxed
            );
        readiness.frameTickThreadId =
            frameThread;
        readiness.inputThreadId =
            inputThread;
        readiness.gameplayLoopSourceVerified = gameplayLoop.sourceVerified;
        readiness.gameplayLoopThreadConsistent = gameplayLoop.threadConsistent;
        readiness.gameplayLoopCompletedCount = gameplayLoop.completed;
        readiness.gameplayLoopThreadId = gameplayLoop.threadId;
        readiness.safeFreeRoamObserved =
            g_safeFreeRoamObserved.load(
                std::memory_order_relaxed
            );
        readiness.roadLookaheadObserved =
            g_roadLookaheadObserved.load(
                std::memory_order_relaxed
            );
        readiness.exactRoadCandidateObserved =
            g_exactRoadCandidateObserved.load(
                std::memory_order_relaxed
            );
        readiness.vehicleSpatialEvidenceObserved =
            g_vehicleSpatialEvidenceObserved.load(
                std::memory_order_relaxed
            );
        readiness.vehicleFootprintVerified =
            g_vehicleFootprintVerified.load(
                std::memory_order_relaxed
            );
        const auto healthCollision = latestWorldCollisionResult();
        readiness.groundEvidenceVerified = healthCollision.available &&
            healthCollision.ground.groundVerified && healthCollision.ground.groundValid;

        // Intentionally false until target-machine calibration/candidate
        // promotion work completes. This keeps construction fail-closed.
        readiness.metricCalibrationVerified = false;
        readiness.spawnCandidateVerified = false;

        const auto readinessReport =
            frr::domain::evaluateMutationReadiness(
                readiness
            );

        {
            std::ostringstream line;
            line << "Construction readiness after " << elapsedSeconds << "s: "
                 << (readinessReport.readyForConstructionExperiment
                     ? "READY"
                     : "BLOCKED")
                 << " blocker="
                 << frr::domain::mutationReadinessBlockerName(
                        readinessReport.blocker
                    )
                 << " gameplayThreadConfirmed="
                 << (readinessReport.gameplayThreadConfirmed ? 1 : 0)
                 << " freeRoamObserved="
                 << (readiness.safeFreeRoamObserved ? 1 : 0)
                 << " roadLookaheadObserved="
                 << (readiness.roadLookaheadObserved ? 1 : 0)
                 << " exactRoadCandidateObserved="
                 << (readiness.exactRoadCandidateObserved ? 1 : 0)
                 << " vehicleSpatialEvidenceObserved="
                 << (readiness.vehicleSpatialEvidenceObserved ? 1 : 0)
                 << " vehicleFootprintVerified="
                 << (readiness.vehicleFootprintVerified ? 1 : 0)
                 << " verifiedFootprintVehicleKey=0x"
                 << std::hex << std::uppercase
                 << g_verifiedFootprintVehicleKey.load(
                        std::memory_order_relaxed
                    )
                 << std::dec
                 << " groundEvidenceVerified="
                 << (readiness.groundEvidenceVerified ? 1 : 0)
                 << " worldOcclusionEvidenceObserved="
                 << (g_worldOcclusionEvidenceObserved.load(
                        std::memory_order_relaxed
                    ) ? 1 : 0)
                 << " metricCalibrationVerified=0"
                 << " spawnCandidateVerified=0";

            Log::instance().info(line.str());
        }

        if (g_renderFrames.load() == 0) {
            Log::instance().warn(
                "No usable EndScene/Present render callback has reached the sampler yet. Motion capture is unavailable."
            );
        }

        if (g_config.inputProbeEnabled && gameplayLoop.completed == 0) {
            Log::instance().warn(
                "No verified gameplay-loop completion observed; fallback challenge input is unavailable."
            );
        }

        if (g_config.frameTickProbeEnabled &&
            g_frameTicks.load() == 0) {
            Log::instance().warn(
                "Opt-in FrameTick diagnostic probe has not fired yet."
            );
        }

        Sleep(30000);
    }
}

} // namespace

RuntimeProbeInstallResult RuntimeProbe::install(
    const RuntimeProbeConfig& config
) {
    g_config = config;
    g_motionCaptureId = GetTickCount64();

    RuntimeProbeInstallResult result{};

    ChallengeInputProbeConfig challengeInputConfig{};
    challengeInputConfig.fallbackVirtualKey =
        config.fallbackChallengeVirtualKey;

    ChallengeInputProbe::configure(
        challengeInputConfig
    );

    {
        std::ostringstream line;
        line << "Challenge fallback input configured: virtualKey=0x"
             << std::hex << std::uppercase
             << ChallengeInputProbe::fallbackVirtualKey()
             << std::dec
             << " (edge-triggered, read-only).";
        Log::instance().info(line.str());
    }

    if (config.worldCollisionDiagnosticsEnabled) {
        if (!config.frameTickProbeEnabled) {
            Log::instance().warn(
                "WorldCollisionDiagnosticsEnabled requires FrameTickProbeEnabled=1 and verified gameplay-loop delivery."
            );
        } else if (!WorldCollisionProbe::addressAvailable()) {
            Log::instance().warn(
                "Verified CheckHitWorld address is not executable in this process. World-collision diagnostics remain fail-closed."
            );
        } else {
            Log::instance().info(
                "World-collision diagnostics armed. Queries require verified gameplay-loop source, completed calls and a consistent FrameTick/gameplay thread."
            );
        }
    }

    if (config.useHornToChallenge) {
        Log::instance().warn(
            "UseHornToChallenge requested, but the verified MW05 action map exposes no native HORN/HONK action yet. The configured fallback key remains the only active challenge input."
        );
    }

    if (config.inputProbeEnabled || config.frameTickProbeEnabled) {
        result.gameplayLoopInstalled = GameplayLoopHook::install(&onGameplayLoopBefore, &onGameplayLoopAfter);
        result.frameTickProbeInstalled = config.frameTickProbeEnabled && result.gameplayLoopInstalled;
        g_frameTickProbeInstalled.store(result.frameTickProbeInstalled);
        Log::instance().info(result.gameplayLoopInstalled ?
            "Verified main-loop observation installed; fallback input uses post-update callback. No native input-poll count is fabricated." :
            "Gameplay-loop observation blocked by discovery/installation evidence; fallback key and gameplay queries remain unavailable.");
    }

    if (config.renderProbeEnabled) {
        result.renderProbeArmed = RenderObservationHook::install(&onRenderFrame);
        Log::instance().info(result.renderProbeArmed ?
            "Guarded D3D9 EndScene/Present observation worker armed; installation and callback delivery are logged separately." :
            "Render observation worker failed to start; motion capture remains unavailable.");
    }

    HANDLE thread = CreateThread(
        nullptr,
        0,
        &healthThread,
        nullptr,
        0,
        nullptr
    );

    if (thread) {
        CloseHandle(thread);
    }

    return result;
}

} // namespace frr::game
