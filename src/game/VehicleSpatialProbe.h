#pragma once

#include "../domain/VehicleSpatialEvidence.h"

#include <cstdint>
#include <vector>

namespace frr::game {

struct VehicleSpatialSnapshot {
    bool registryComplete = false;

    // Public MW05 reconstruction evidence shows rigid-body GetDimension()
    // feeding collision boxes and bottom/scrape offsets as local half-extents.
    // Target runtime logs still sanity-check the actual values.
    bool halfExtentSemanticsResearchBacked = true;

    std::uint32_t registryCount = 0;
    std::uint32_t enabledActiveVehicles = 0;
    std::uint32_t ignoredInactiveVehicles = 0;
    std::uint32_t failedSpatialReads = 0;

    std::vector<frr::domain::VehicleOrientedBox> boxes{};
};

class VehicleSpatialProbe {
public:
    static VehicleSpatialSnapshot sample();
};

} // namespace frr::game
