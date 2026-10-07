#include "SpawnSafety.h"

#include <algorithm>

namespace frr::domain {

SpawnDecision evaluateSpawnEnvironment(
    const SpawnEnvironmentInput& input,
    const SpawnSafetyTuning& tuning
) {
    if (!input.experimentalFeatureEnabled) {
        return {false, SpawnRejectReason::ExperimentalFeatureDisabled};
    }

    if (!input.supportedExecutable) {
        return {false, SpawnRejectReason::UnsupportedExecutable};
    }

    if (!input.freeRoamCandidate) {
        return {false, SpawnRejectReason::NotFreeRoam};
    }

    if (input.loading) {
        return {false, SpawnRejectReason::Loading};
    }

    if (input.inNIS) {
        return {false, SpawnRejectReason::NisActive};
    }

    if (input.fade) {
        return {false, SpawnRejectReason::FadeActive};
    }

    if (!input.playerAvailable) {
        return {false, SpawnRejectReason::PlayerUnavailable};
    }

    if (!input.independentPlayerCrossCheck) {
        return {false, SpawnRejectReason::PlayerCrossCheckFailed};
    }

    if (!input.roadNetworkAvailable) {
        return {false, SpawnRejectReason::RoadNetworkUnavailable};
    }

    if (input.stableFreeRoamSamples <
        std::max(tuning.requiredStableSamples, 1u)) {
        return {false, SpawnRejectReason::StabilityWindowNotMet};
    }

    if (input.maxLiveRivals <= 0 ||
        input.liveRivals >= input.maxLiveRivals) {
        return {false, SpawnRejectReason::PopulationBudgetFull};
    }

    return {true, SpawnRejectReason::None};
}

SpawnDecision evaluateSpawnCandidate(
    const SpawnEnvironmentInput& environment,
    const SpawnCandidateInput& candidate,
    const SpawnSafetyTuning& tuning
) {
    const SpawnDecision env =
        evaluateSpawnEnvironment(environment, tuning);

    if (!env.allowed) {
        return env;
    }

    if (!candidate.vehicleAvailable) {
        return {false, SpawnRejectReason::VehicleUnavailable};
    }

    if (!candidate.available) {
        return {false, SpawnRejectReason::CandidateUnavailable};
    }

    if (!candidate.roadValid) {
        return {false, SpawnRejectReason::RoadCandidateInvalid};
    }

    if (!candidate.groundValid) {
        return {false, SpawnRejectReason::GroundInvalid};
    }

    if (candidate.overlapsLiveVehicle) {
        return {false, SpawnRejectReason::VehicleOverlap};
    }

    if (!candidate.metricDistanceVerified) {
        return {
            false,
            SpawnRejectReason::DistanceScaleUnverified
        };
    }

    const float minimumDistance =
        std::max(tuning.absoluteMinDistanceMeters, 0.0f);

    if (candidate.distanceFromPlayerMeters < minimumDistance) {
        return {false, SpawnRejectReason::TooClose};
    }

    if (candidate.visibleToPlayer) {
        return {false, SpawnRejectReason::VisiblePopInRisk};
    }

    const float hiddenMaximum = std::max(
        tuning.hiddenCandidateMaxDistanceMeters,
        minimumDistance
    );

    if (candidate.distanceFromPlayerMeters > hiddenMaximum &&
        !candidate.streamingVerified) {
        return {
            false,
            SpawnRejectReason::TooFarWithoutStreamingProof
        };
    }

    return {true, SpawnRejectReason::None};
}

const char* spawnRejectReasonName(SpawnRejectReason reason) {
    switch (reason) {
        case SpawnRejectReason::None: return "None";
        case SpawnRejectReason::ExperimentalFeatureDisabled: return "ExperimentalFeatureDisabled";
        case SpawnRejectReason::UnsupportedExecutable: return "UnsupportedExecutable";
        case SpawnRejectReason::NotFreeRoam: return "NotFreeRoam";
        case SpawnRejectReason::Loading: return "Loading";
        case SpawnRejectReason::NisActive: return "NisActive";
        case SpawnRejectReason::FadeActive: return "FadeActive";
        case SpawnRejectReason::PlayerUnavailable: return "PlayerUnavailable";
        case SpawnRejectReason::PlayerCrossCheckFailed: return "PlayerCrossCheckFailed";
        case SpawnRejectReason::RoadNetworkUnavailable: return "RoadNetworkUnavailable";
        case SpawnRejectReason::StabilityWindowNotMet: return "StabilityWindowNotMet";
        case SpawnRejectReason::PopulationBudgetFull: return "PopulationBudgetFull";
        case SpawnRejectReason::VehicleUnavailable: return "VehicleUnavailable";
        case SpawnRejectReason::CandidateUnavailable: return "CandidateUnavailable";
        case SpawnRejectReason::RoadCandidateInvalid: return "RoadCandidateInvalid";
        case SpawnRejectReason::GroundInvalid: return "GroundInvalid";
        case SpawnRejectReason::VehicleOverlap: return "VehicleOverlap";
        case SpawnRejectReason::DistanceScaleUnverified: return "DistanceScaleUnverified";
        case SpawnRejectReason::TooClose: return "TooClose";
        case SpawnRejectReason::VisiblePopInRisk: return "VisiblePopInRisk";
        case SpawnRejectReason::TooFarWithoutStreamingProof: return "TooFarWithoutStreamingProof";
    }

    return "Unknown";
}

} // namespace frr::domain
