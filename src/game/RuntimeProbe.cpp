#include "RuntimeProbe.h"

#include "ChallengeInputProbe.h"
#include "GameBridge.h"
#include "RoadCandidateProbe.h"
#include "VehicleCatalogProbe.h"
#include "VehicleSpatialProbe.h"
#include "../core/Log.h"
#include "../domain/MotionScaleObserver.h"
#include "../domain/MutationReadiness.h"
#include "../domain/RoadCandidatePlanner.h"
#include "../domain/RuntimeSession.h"
#include "../domain/SpawnSafety.h"
#include "../domain/UndergroundBlacklist.h"
#include "../domain/VehicleFootprintLearning.h"
#include "../domain/VehicleSelection.h"
#include "../domain/VehicleSpatialEvidence.h"
#include "../persistence/UndergroundBlacklistStore.h"

#include <nfsmw_sdk/d3d9_hooks.h>
#include <nfsmw_sdk/functions.h>
#include <nfsmw_sdk/input.h>
#include <nfsmw_sdk/midhook.h>

#include <windows.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>
#include <string_view>

namespace frr::game {
namespace {

RuntimeProbeConfig g_config{};
RuntimeSnapshot g_last{};
bool g_haveLast = false;

std::atomic<std::uint64_t> g_renderFrames{0};
std::atomic<std::uint64_t> g_inputPolls{0};
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
std::atomic<std::uint32_t> g_verifiedFootprintVehicleKey{0};

frr::domain::VehicleFootprintLearner g_vehicleFootprintLearner{};

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

bool g_haveSpawnPreflight = false;
frr::domain::SpawnRejectReason g_lastSpawnPreflightReason =
    frr::domain::SpawnRejectReason::ExperimentalFeatureDisabled;

std::uint32_t findFirstVerifiedCatalogFootprintKey() {
    const auto& catalog =
        frr::domain::defaultVehicleCatalog();

    for (const auto& definition : catalog) {
        const auto runtimeKey =
            VehicleCatalogProbe::runtimeKeyForName(
                definition.key
            );

        if (!runtimeKey) {
            continue;
        }

        const auto estimate =
            g_vehicleFootprintLearner.estimate(
                *runtimeKey
            );

        if (estimate.verified) {
            return *runtimeKey;
        }
    }

    return 0;
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
        current.mode == WorldProbeMode::FreeRoamCandidate &&
        current.capabilities.canClassifyFreeRoam;
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

    if (current.mode == WorldProbeMode::FreeRoamCandidate &&
        current.capabilities.canClassifyFreeRoam) {
        g_safeFreeRoamObserved.store(
            true,
            std::memory_order_relaxed
        );
    }

    const auto roadCandidates =
        RoadCandidateProbe::build(
            current.roadNavigation
        );

    VehicleSpatialSnapshot vehicleSpatial{};
    if (current.inWorld &&
        current.vehicles.registryReadable) {
        vehicleSpatial =
            VehicleSpatialProbe::sample();
    }

    if (current.roadNavigation.available) {
        g_roadLookaheadObserved.store(
            true,
            std::memory_order_relaxed
        );
    }

    if (vehicleSpatial.registryComplete &&
        vehicleSpatial.failedSpatialReads == 0) {
        g_vehicleSpatialEvidenceObserved.store(
            true,
            std::memory_order_relaxed
        );
    }

    for (const auto& box : vehicleSpatial.boxes) {
        if (box.valid) {
            g_vehicleFootprintLearner.observe(box);
        }
    }

    if (g_verifiedFootprintVehicleKey.load(
            std::memory_order_relaxed) == 0 &&
        g_vehicleFootprintLearner.verifiedModelCount() > 0) {
        const std::uint32_t verifiedCatalogKey =
            findFirstVerifiedCatalogFootprintKey();

        if (verifiedCatalogKey != 0) {
            g_verifiedFootprintVehicleKey.store(
                verifiedCatalogKey,
                std::memory_order_relaxed
            );
            g_vehicleFootprintVerified.store(
                true,
                std::memory_order_relaxed
            );

            const auto estimate =
                g_vehicleFootprintLearner.estimate(
                    verifiedCatalogKey
                );

            std::ostringstream line;
            line << "Verified pre-construction vehicle footprint:"
                 << " vehicleKey=0x"
                 << std::hex << std::uppercase
                 << verifiedCatalogKey
                 << std::dec
                 << " samples="
                 << estimate.sampleCount
                 << " halfExtentsWorld=("
                 << std::fixed << std::setprecision(3)
                 << estimate.meanHalfExtents.x << ","
                 << estimate.meanHalfExtents.y << ","
                 << estimate.meanHalfExtents.z << ")"
                 << " maxRelativeSpread="
                 << estimate.maximumObservedRelativeSpread;

            Log::instance().info(line.str());
        }
    }

    bool exactRoadCandidateThisSample = false;
    for (const auto& candidate : roadCandidates) {
        if (frr::domain::inspectRoadCandidate(candidate) ==
            frr::domain::RoadCandidateBlocker::None) {
            exactRoadCandidateThisSample = true;
            break;
        }
    }

    if (exactRoadCandidateThisSample) {
        g_exactRoadCandidateObserved.store(
            true,
            std::memory_order_relaxed
        );
    }

    updateRuntimeSessionAndSpawnPreflight(current);
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

                const std::uint32_t footprintKey =
                    g_verifiedFootprintVehicleKey.load(
                        std::memory_order_relaxed
                    );

                if (footprintKey != 0 &&
                    candidate.forward.finite) {
                    const auto footprintEstimate =
                        g_vehicleFootprintLearner.estimate(
                            footprintKey
                        );

                    if (footprintEstimate.verified) {
                        const auto candidateFootprint =
                            frr::domain::
                                makeRoadAlignedVehicleFootprint(
                                    footprintKey,
                                    {
                                        candidate.position.x,
                                        candidate.position.y,
                                        candidate.position.z
                                    },
                                    {
                                        candidate.forward.x,
                                        candidate.forward.y,
                                        candidate.forward.z
                                    },
                                    footprintEstimate
                                        .meanHalfExtents
                                );

                        const auto footprintOverlap =
                            frr::domain::
                                evaluateFootprintAgainstFleet(
                                    candidateFootprint,
                                    vehicleSpatial.boxes,
                                    vehicleSpatial
                                        .registryComplete
                                );

                        line << ",footprintKey=0x"
                             << std::hex << std::uppercase
                             << footprintKey
                             << std::dec
                             << ",preconstructionOverlap="
                             << (footprintOverlap.verified
                                 ? (footprintOverlap.overlaps
                                    ? "occupied"
                                    : "clear")
                                 : "unverified")
                             << ",footprintFleetChecked="
                             << footprintOverlap
                                    .checkedVehicles
                             << ",footprintFleetInvalid="
                             << footprintOverlap
                                    .invalidVehicles;
                    }
                }
            }

            line << "]";
        }

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

void NFSMW_CDECL onFrameTickProbe(nfsmw_regs*) {
    rememberThread(g_frameTickThreadId);
    ++g_frameTicks;
}

void onRenderFrame(void*) {
    rememberThread(g_renderThreadId);
    const std::uint64_t frame = ++g_renderFrames;

    if (g_config.sampleEveryFrames == 0 ||
        frame == 1 ||
        (frame % g_config.sampleEveryFrames) == 0) {
        sampleAndLog(frame);
    }
}

void onInputPoll() {
    rememberThread(g_inputThreadId);
    ChallengeInputProbe::onPoll();
    ++g_inputPolls;
}

DWORD WINAPI healthThread(LPVOID) {
    // We only inspect our own atomics here. No engine objects are
    // dereferenced from this background thread.
    Sleep(8000);

    std::ostringstream out;
    const DWORD renderThread = g_renderThreadId.load();
    const DWORD inputThread = g_inputThreadId.load();
    const DWORD frameThread = g_frameTickThreadId.load();

    out << "Runtime hook health after 8s:"
        << " renderFrames=" << g_renderFrames.load()
        << " inputPolls=" << g_inputPolls.load()
        << " frameTicks=" << g_frameTicks.load()
        << " samples=" << g_samples.load()
        << " challengeFallbackPresses="
        << ChallengeInputProbe::totalPresses()
        << " renderThread=" << renderThread
        << " inputThread=" << inputThread
        << " frameTickThread=" << frameThread;

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
        line << "Construction readiness after 8s: "
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
             << " metricCalibrationVerified=0"
             << " spawnCandidateVerified=0";

        Log::instance().info(line.str());
    }

    if (g_renderFrames.load() == 0) {
        Log::instance().warn(
            "D3D9 EndScene callback has not fired yet."
        );
    }

    if (g_inputPolls.load() == 0) {
        Log::instance().warn(
            "Input-poll diagnostic hook has not fired yet."
        );
    }

    if (g_config.frameTickProbeEnabled &&
        g_frameTicks.load() == 0) {
        Log::instance().warn(
            "Opt-in FrameTick diagnostic probe has not fired yet."
        );
    }

    return 0;
}

} // namespace

RuntimeProbeInstallResult RuntimeProbe::install(
    const RuntimeProbeConfig& config
) {
    g_config = config;

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

    if (config.useHornToChallenge) {
        Log::instance().warn(
            "UseHornToChallenge requested, but the verified MW05 action map exposes no native HORN/HONK action yet. The configured fallback key remains the only active challenge input."
        );
    }

    if (config.renderProbeEnabled) {
        nfsmw_d3d9_install(&onRenderFrame, nullptr);
        result.renderProbeArmed = true;

        Log::instance().info(
            "D3D9 EndScene runtime probe armed (read-only)."
        );
    }

    if (config.inputProbeEnabled) {
        result.inputProbeInstalled =
            nfsmw::input::on_poll([]() {
                onInputPoll();
            });

        if (result.inputProbeInstalled) {
            Log::instance().info(
                "Input-poll diagnostic hook installed."
            );
        } else {
            Log::instance().warn(
                "Input-poll diagnostic hook failed to install."
            );
        }
    }

    if (config.frameTickProbeEnabled) {
        static nfsmw::MidHook frameTickHook(
            NFSMW_FN_GameFrameTick_MainLoopUpdate,
            &onFrameTickProbe
        );

        result.frameTickProbeInstalled =
            frameTickHook.installed();

        g_frameTickProbeInstalled.store(
            result.frameTickProbeInstalled,
            std::memory_order_relaxed
        );

        if (result.frameTickProbeInstalled) {
            Log::instance().info(
                "Opt-in GameFrameTick diagnostic mid-hook installed (read-only register-preserving probe)."
            );
        } else {
            Log::instance().warn(
                "GameFrameTick diagnostic probe failed to install; no gameplay mutation will use this path."
            );
        }
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
