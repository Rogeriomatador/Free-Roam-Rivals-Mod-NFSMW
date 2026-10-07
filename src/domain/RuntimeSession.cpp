#include "RuntimeSession.h"

#include <limits>

namespace frr::domain {

RuntimeSessionSnapshot RuntimeSessionTracker::tick(
    const RuntimeSessionObservation& observation
) {
    const bool safe =
        observation.safeFreeRoam &&
        observation.playerIdentity != 0 &&
        observation.roadNetworkIdentity != 0;

    bool newGeneration = false;

    if (!safe) {
        active_ = false;
        stableSamples_ = 0;
        playerIdentity_ = 0;
        roadNetworkIdentity_ = 0;
        return snapshot();
    }

    const bool identityChanged =
        active_ &&
        (playerIdentity_ != observation.playerIdentity ||
         roadNetworkIdentity_ != observation.roadNetworkIdentity);

    if (!active_ || identityChanged) {
        if (generation_ != std::numeric_limits<std::uint64_t>::max()) {
            ++generation_;
        }

        stableSamples_ = 1;
        active_ = true;
        newGeneration = true;
    } else if (stableSamples_ != std::numeric_limits<unsigned>::max()) {
        ++stableSamples_;
    }

    playerIdentity_ = observation.playerIdentity;
    roadNetworkIdentity_ = observation.roadNetworkIdentity;

    RuntimeSessionSnapshot out = snapshot();
    out.newGeneration = newGeneration;
    return out;
}

RuntimeSessionSnapshot RuntimeSessionTracker::snapshot() const {
    RuntimeSessionSnapshot out{};
    out.generation = generation_;
    out.stableSamples = stableSamples_;
    out.active = active_;
    return out;
}

void RuntimeSessionTracker::reset() {
    generation_ = 0;
    stableSamples_ = 0;
    active_ = false;
    playerIdentity_ = 0;
    roadNetworkIdentity_ = 0;
}

} // namespace frr::domain
