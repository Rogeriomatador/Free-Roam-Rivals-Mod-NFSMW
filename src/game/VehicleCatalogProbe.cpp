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
