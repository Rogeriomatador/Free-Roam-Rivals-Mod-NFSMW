#include "VehicleCatalogProbe.h"

#include "../domain/VehicleSelection.h"

#include <windows.h>

#include <NFSPluginSDK/Game.MW05/Types/Attrib/Gen/pvehicle.h>

#include <string>

namespace frr::game {
namespace {

bool vehicleKeyExists(const char* key) {
#if defined(_MSC_VER)
    __try {
#endif
        const auto instance =
            NFSPluginSDK::MW05::Attrib::Gen::pvehicle::
                TryGetInstance(key);

        return instance.mCollection != nullptr;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
#endif
}

} // namespace

std::optional<std::uint32_t>
VehicleCatalogProbe::runtimeKeyForName(
    std::string_view key
) {
    if (key.empty()) {
        return std::nullopt;
    }

    const std::string owned(key);

#if defined(_MSC_VER)
    __try {
#endif
        const auto instance =
            NFSPluginSDK::MW05::Attrib::Gen::pvehicle::
                TryGetInstance(owned.c_str());

        if (!instance.mCollection) {
            return std::nullopt;
        }

        const auto runtimeKey =
            NFSPluginSDK::MW05::Attrib::StringToKey(
                owned.c_str()
            );

        if (runtimeKey == 0) {
            return std::nullopt;
        }

        return runtimeKey;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return std::nullopt;
    }
#endif
}

VehicleCatalogProbeResult
VehicleCatalogProbe::validateDefaultCatalog() {
    VehicleCatalogProbeResult out{};

    const auto& catalog =
        domain::defaultVehicleCatalog();

    out.configured = catalog.size();

    for (const auto& vehicle : catalog) {
        const std::string key(vehicle.key);

        if (vehicleKeyExists(key.c_str())) {
            ++out.available;
        } else {
            out.missingKeys.push_back(key);
        }
    }

    out.completed = true;
    return out;
}

} // namespace frr::game
