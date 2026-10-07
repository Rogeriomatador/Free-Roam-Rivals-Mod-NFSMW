#pragma once

#include <cstdint>

namespace frr::domain {

struct RuntimeSessionObservation {
    bool safeFreeRoam = false;
    std::uintptr_t playerIdentity = 0;
    std::uintptr_t roadNetworkIdentity = 0;
};

struct RuntimeSessionSnapshot {
    std::uint64_t generation = 0;
    unsigned stableSamples = 0;
    bool active = false;
    bool newGeneration = false;
};

class RuntimeSessionTracker {
public:
    RuntimeSessionSnapshot tick(
        const RuntimeSessionObservation& observation
    );

    RuntimeSessionSnapshot snapshot() const;
    void reset();

private:
    std::uint64_t generation_ = 0;
    unsigned stableSamples_ = 0;
    bool active_ = false;
    std::uintptr_t playerIdentity_ = 0;
    std::uintptr_t roadNetworkIdentity_ = 0;
};

} // namespace frr::domain
