#include "VehicleCatalogProbe.h"

#include "../domain/VehicleSelection.h"

#include <NFSPluginSDK/Game.MW05/Types/Attrib/Gen/pvehicle.h>

#include <string>

namespace frr::game {

VehicleCatalogProbeResult
VehicleCatalogProbe::validateDefaultCatalog() {
    VehicleCatalogProbeResult out{};

    const auto& catalog =
        domain::defaultVehicleCatalog();

    out.configured = catalog.size();

#if defined(_MSC_VER)
    __try {
#endif
        for (const auto& vehicle : catalog) {
            const std::string key(vehicle.key);

            const auto instance =
                NFSPluginSDK::MW05::Attrib::Gen::pvehicle::
                    TryGetInstance(key.c_str());

            if (instance.mCollection) {
                ++out.available;
            } else {
                out.missingKeys.push_back(key);
            }
        }

        out.completed = true;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out.completed = false;
        out.available = 0;
        out.missingKeys.clear();
    }
#endif

    return out;
}

} // namespace frr::game
