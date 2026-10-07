#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
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

    // Returns the exact MW05 pvehicle collection key used by IVehicle::GetVehicleKey.
    // The configured collection must exist in the live attribute database.
    static std::optional<std::uint32_t> runtimeKeyForName(
        std::string_view key
    );
};

} // namespace frr::game
