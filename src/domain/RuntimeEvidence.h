#pragma once
#include <cstdint>

namespace frr::domain {
// Value-only lease. No engine pointer is dereferenced by this domain layer.
struct RuntimeEvidenceStamp {
    bool safeFreeRoam = false;
    std::uint64_t generation = 0;
    std::uintptr_t playerIVehicle = 0;
    std::uintptr_t playerPVehicle = 0;
    std::uintptr_t roadNetwork = 0;
    std::uintptr_t raceStatus = 0;
    std::uint64_t profileKey = 0;
    std::uint64_t capturedAtMillis = 0;
};

bool runtimeEvidenceUsable(
    const RuntimeEvidenceStamp& captured,
    const RuntimeEvidenceStamp& current,
    std::uint64_t nowMillis,
    std::uint64_t maximumAgeMillis = 500
);
} // namespace frr::domain
