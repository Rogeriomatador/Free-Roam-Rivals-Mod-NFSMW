#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace frr::game {

struct VehicleCatalogProbeResult {
    bool completed = false;
    std::size_t configured = 0;
    std::size_t available = 0;
    std::vector<std::string> missingKeys;
};

class VehicleCatalogProbe {
public:
    static VehicleCatalogProbeResult validateDefaultCatalog();
};

} // namespace frr::game
