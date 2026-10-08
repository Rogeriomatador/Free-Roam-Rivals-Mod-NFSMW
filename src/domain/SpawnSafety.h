#pragma once

#include <cstdint>

namespace frr::domain {

enum class SpawnRejectReason {
    None,
    ExperimentalFeatureDisabled,
    UnsupportedExecutable,
    NotFreeRoam,
    Loading,
    NisActive,
    FadeActive,
    PlayerUnavailable,
    PlayerCrossCheckFailed,
    RoadNetworkUnavailable,
    PursuitStateUnavailable,
    PursuitUnsafe,
    StabilityWindowNotMet,
    PopulationBudgetFull,
    VehicleUnavailable,
    CandidateUnavailable,
    RoadCandidateInvalid,
    GroundInvalid,
    VehicleOverlap,
    DistanceScaleUnverified,
    TooClose,
    VisiblePopInRisk,
    TooFarWithoutStreamingProof
};

struct SpawnSafetyTuning {
    float absoluteMinDistanceMeters = 300.0f;
    float hiddenCandidateMaxDistanceMeters = 850.0f;
    unsigned requiredStableSamples = 6;
};

// Clear requires verified fresh native evidence, not the absence of cop cars.
enum class PursuitSafetyState { Unknown, Clear, Active, Cooldown, Busted };

struct SpawnEnvironmentInput {
    PursuitSafetyState pursuitState = PursuitSafetyState::Unknown;
    bool experimentalFeatureEnabled = false;
    bool supportedExecutable = false;
    bool freeRoamCandidate = false;
    bool loading = false;
    bool inNIS = false;
    bool fade = false;
    bool playerAvailable = false;
    bool independentPlayerCrossCheck = false;
    bool roadNetworkAvailable = false;
    unsigned stableFreeRoamSamples = 0;
    int liveRivals = 0;
    int maxLiveRivals = 1;
};

struct SpawnCandidateInput {
    bool available = false;
    bool vehicleAvailable = false;
    bool roadValid = false;
    bool groundValid = false;
    bool overlapsLiveVehicle = false;
    bool visibleToPlayer = true;
    bool streamingVerified = false;

    // Must be true only when the adapter has converted engine/world
    // coordinates through a separately verified world-units-to-metre scale.
    bool metricDistanceVerified = false;
    float distanceFromPlayerMeters = 0.0f;
};

struct SpawnDecision {
    bool allowed = false;
    SpawnRejectReason reason = SpawnRejectReason::NotFreeRoam;
};

SpawnDecision evaluateSpawnEnvironment(
    const SpawnEnvironmentInput& input,
    const SpawnSafetyTuning& tuning = {}
);

SpawnDecision evaluateSpawnCandidate(
    const SpawnEnvironmentInput& environment,
    const SpawnCandidateInput& candidate,
    const SpawnSafetyTuning& tuning = {}
);

const char* spawnRejectReasonName(SpawnRejectReason reason);

} // namespace frr::domain
