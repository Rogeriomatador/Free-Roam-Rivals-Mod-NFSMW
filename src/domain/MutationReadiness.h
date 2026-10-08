#pragma once

#include <cstdint>

namespace frr::domain {

enum class MutationReadinessBlocker {
    None,
    FrameTickProbeDisabled,
    FrameTickProbeNotInstalled,
    FrameTickNotObserved,
    InputPollNotObserved,
    MainLoopThreadUnconfirmed,
    FreeRoamNotObserved,
    RoadLookaheadUnavailable,
    ExactRoadCandidateUnavailable,
    VehicleSpatialEvidenceUnavailable,
    VehicleFootprintUnavailable,
    GroundEvidenceUnavailable,
    MetricCalibrationUnverified,
    SpawnCandidateUnverified,
    PursuitSafetyUnverified
};

struct MutationReadinessInput {
    bool frameTickProbeEnabled = false;
    bool frameTickProbeInstalled = false;

    std::uint64_t frameTickCount = 0;
    std::uint64_t inputPollCount = 0;

    std::uint32_t frameTickThreadId = 0;
    std::uint32_t inputThreadId = 0;

    // Explicit alternative to the old unobserved input-poller anchor. This
    // requires the WFP CALL ABI + pinned GameFrameTick target to agree.
    bool gameplayLoopSourceVerified = false;
    bool gameplayLoopThreadConsistent = false;
    std::uint64_t gameplayLoopCompletedCount = 0;
    std::uint32_t gameplayLoopThreadId = 0;

    bool safeFreeRoamObserved = false;
    bool roadLookaheadObserved = false;
    bool exactRoadCandidateObserved = false;
    bool vehicleSpatialEvidenceObserved = false;
    bool vehicleFootprintVerified = false;
    bool groundEvidenceVerified = false;
    bool metricCalibrationVerified = false;
    bool spawnCandidateVerified = false;
    // Fresh exact-target pursuit state must be verified Clear before readiness.
    bool pursuitClearVerified = false;
};

struct MutationReadinessReport {
    bool gameplayThreadConfirmed = false;
    bool readyForConstructionExperiment = false;
    MutationReadinessBlocker blocker =
        MutationReadinessBlocker::FrameTickProbeDisabled;
};

MutationReadinessReport evaluateMutationReadiness(
    const MutationReadinessInput& input
);

const char* mutationReadinessBlockerName(
    MutationReadinessBlocker blocker
);

} // namespace frr::domain

