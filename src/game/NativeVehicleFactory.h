#pragma once
#include "NfsPluginCoordinateAdapter.h"
#include "../domain/SpawnSafety.h"
#include "../domain/NativeRoadSeed.h"
#include "../domain/VehicleSpatialEvidence.h"
#include <vector>
#include <cstdint>

namespace frr::game {
struct NativeRoadTarget {
    domain::NativeRoadSeed seed{};
    CanonicalMwVector3 position{}, forward{};
    std::uintptr_t player = 0, road = 0, race = 0, segmentTable = 0;
    std::uint64_t profile = 0, millis = 0;
};
struct NativeRoadCaptureReport {
    const char* status = "not_sampled";
    unsigned liveSlots = 0, sampledSlots = 0, accepted = 0;
    // 0 success; 1 vehicle; 2 driver; 3 AI contract; 4 nav pointer;
    // 5 nav memory; 6 invalid scalar seed; 7 unstable seed; 8 geometry; 9 fault.
    unsigned rejected[10]{};
    unsigned rejectedContext = 0;
};
struct NativeOwnedSnapshot {
    bool owned = false, available = false, contextMatches = false;
    bool loading = false, active = false, destroyed = false, racerPrepared = false, roadPrepared = false;
    domain::PursuitSafetyState pursuit = domain::PursuitSafetyState::Unknown;
    domain::VehicleOrientedBox box{};
    float speed = 0.0f;
};
struct NativeFactoryRequest {
    domain::SpawnEnvironmentInput environment{};
    domain::SpawnCandidateInput candidate{};
    CanonicalMwVector3 position{};
    CanonicalMwVector3 forward{};
    std::uint32_t vehicleKey = 0;
    NativeRoadTarget roadTarget{};
};
enum class NativeFactoryResult {
    Blocked, CompatibilityBlocked, ConstructedInactive, RacerPreparedInactive,
    RoadPreparedInactive, Activated,
    RemovalRequested, RemovalPending, Removed, RemovedByEngine, Faulted
};
// One owned object maximum; every engine operation requires a current verified
// outer post-update callback, fresh identity and compatible world context.
class NativeVehicleFactory {
public:
    static domain::PursuitSafetyState pursuitState();
    static std::vector<NativeRoadTarget> captureRoadTargets(std::size_t& batchIndex,
        NativeRoadCaptureReport& report);
    static NativeOwnedSnapshot snapshot();
    static NativeFactoryResult constructInactive(const NativeFactoryRequest& request);
    static NativeFactoryResult prepareRacerInactive();
    static NativeFactoryResult resetRoadInactive();
    static NativeFactoryResult activatePrepared(const domain::SpawnEnvironmentInput& environment,
        const domain::SpawnCandidateInput& candidate);
    static NativeFactoryResult requestRemoval();
    static NativeFactoryResult observeRemoval();
    static NativeFactoryResult observeExternalRemoval();
};
}
