#pragma once
#include "NfsPluginCoordinateAdapter.h"
#include "../domain/SpawnSafety.h"
#include <cstdint>

namespace frr::game {
struct NativeFactoryRequest {
    domain::SpawnEnvironmentInput environment{};
    domain::SpawnCandidateInput candidate{};
    CanonicalMwVector3 position{};
    CanonicalMwVector3 forward{};
    std::uint32_t vehicleKey = 0;
};
enum class NativeFactoryResult {
    Blocked, ConstructedInactive, RacerPreparedInactive,
    RemovalRequested, RemovalPending, Removed, Faulted
};
// One owned object maximum. This adapter is intentionally not dispatched by
// RuntimeProbe yet: owned road-navigation initialization and movement/cleanup
// validation must precede a playable experimental build.
class NativeVehicleFactory {
public:
    static NativeFactoryResult constructInactive(const NativeFactoryRequest& request);
    static NativeFactoryResult prepareRacerInactive();
    static NativeFactoryResult requestRemoval();
    static NativeFactoryResult observeRemoval();
};
}
