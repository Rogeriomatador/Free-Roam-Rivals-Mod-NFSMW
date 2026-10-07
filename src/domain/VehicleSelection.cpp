#include "VehicleSelection.h"

#include <algorithm>
#include <limits>
#include <vector>

namespace frr::domain {
namespace {

std::uint64_t splitmix64(std::uint64_t x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

int effectiveWeight(
    const VehicleDefinition& v,
    const VehicleSelectionContext& c,
    bool ignoreAvoid
) {
    if (v.tier < c.minimumTier || v.tier > c.maximumTier) {
        return 0;
    }

    if (!c.allowProcedural && v.procedural) {
        return 0;
    }

    if (v.legendary && !c.allowLegendary) {
        return 0;
    }

    if (v.special && !c.allowSpecial) {
        return 0;
    }

    if (!ignoreAvoid &&
        !c.avoidKey.empty() &&
        v.key == c.avoidKey) {
        return 0;
    }

    int weight = std::max(v.baseWeight, 0);

    // Districts are a preference, never a hard lock. That keeps the
    // population varied while still making each area feel different.
    if (c.district != DistrictNone &&
        (v.preferredDistricts & c.district) != 0u) {
        weight = std::min(
            weight * 2,
            std::numeric_limits<int>::max()
        );
    }

    return weight;
}

std::optional<VehicleSelectionResult> selectPass(
    const std::vector<VehicleDefinition>& catalog,
    const VehicleSelectionContext& context,
    bool ignoreAvoid
) {
    std::uint64_t total = 0;

    for (const auto& v : catalog) {
        const int w = effectiveWeight(v, context, ignoreAvoid);
        if (w > 0) {
            total += static_cast<std::uint64_t>(w);
        }
    }

    if (total == 0) {
        return std::nullopt;
    }

    const std::uint64_t random = splitmix64(context.seed);
    std::uint64_t roll = random % total;

    for (std::size_t i = 0; i < catalog.size(); ++i) {
        const int weight =
            effectiveWeight(catalog[i], context, ignoreAvoid);

        if (weight <= 0) {
            continue;
        }

        const auto w = static_cast<std::uint64_t>(weight);

        if (roll < w) {
            return VehicleSelectionResult{i, random};
        }

        roll -= w;
    }

    return std::nullopt;
}

} // namespace

const std::vector<VehicleDefinition>& defaultVehicleCatalog() {
    // Internal pvehicle keys below are the vanilla MW05 collection names
    // used by public MW05 tooling/community projects.
    static const std::vector<VehicleDefinition> catalog = {
        // Tier 1 — starter / local street cars.
        {"cobaltss", "Chevrolet Cobalt SS", 1, 110, DistrictRosewood | DistrictCamden, true, false, false},
        {"punto", "Fiat Punto", 1, 105, DistrictRosewood, true, false, false},
        {"gti", "Volkswagen Golf GTI", 1, 110, DistrictRosewood | DistrictCamden, true, false, false},
        {"is300", "Lexus IS 300", 1, 95, DistrictRosewood | DistrictDowntown, true, false, false},
        {"a3", "Audi A3", 1, 90, DistrictRosewood | DistrictDowntown, true, false, false},
        {"clio", "Renault Clio", 1, 80, DistrictRosewood, true, false, false},

        // Tier 2 — established street racers.
        {"rx8", "Mazda RX-8", 2, 110, DistrictRosewood | DistrictCamden, true, false, false},
        {"eclipsegt", "Mitsubishi Eclipse GT", 2, 110, DistrictCamden, true, false, false},
        {"tt", "Audi TT", 2, 90, DistrictDowntown | DistrictRosewood, true, false, false},
        {"a4", "Audi A4", 2, 85, DistrictDowntown, true, false, false},
        {"mustanggt", "Ford Mustang GT", 2, 105, DistrictCamden | DistrictHighway, true, false, false},
        {"gto", "Pontiac GTO", 2, 100, DistrictCamden | DistrictHighway, true, false, false},
        {"monaro", "Vauxhall Monaro VXR", 2, 85, DistrictCamden, true, false, false},
        {"clk500", "Mercedes-Benz CLK 500", 2, 75, DistrictDowntown | DistrictHighway, true, false, false},
        {"cts", "Cadillac CTS", 2, 75, DistrictDowntown, true, false, false},

        // Tier 3 — serious tuned/performance cars.
        {"supra", "Toyota Supra", 3, 110, DistrictCamden | DistrictHighway, true, false, false},
        {"rx7", "Mazda RX-7", 3, 110, DistrictCamden | DistrictDowntown, true, false, false},
        {"imprezawrx", "Subaru Impreza WRX STI", 3, 100, DistrictRosewood | DistrictCamden, true, false, false},
        {"lancerevo8", "Mitsubishi Lancer Evolution VIII", 3, 100, DistrictCamden | DistrictDowntown, true, false, false},
        {"elise", "Lotus Elise", 3, 85, DistrictDowntown, true, false, false},
        {"caymans", "Porsche Cayman S", 3, 90, DistrictDowntown | DistrictHighway, true, false, false},
        {"sl500", "Mercedes-Benz SL 500", 3, 75, DistrictDowntown | DistrictHighway, true, false, false},

        // Tier 4 — elite street cars.
        {"997s", "Porsche 911 Carrera S", 4, 95, DistrictDowntown | DistrictHighway, true, false, false},
        {"corvette", "Chevrolet Corvette C6", 4, 100, DistrictCamden | DistrictHighway, true, false, false},
        {"db9", "Aston Martin DB9", 4, 80, DistrictDowntown | DistrictHighway, true, false, false},
        {"gallardo", "Lamborghini Gallardo", 4, 90, DistrictDowntown | DistrictHighway, true, false, false},
        {"viper", "Dodge Viper SRT10", 4, 90, DistrictCamden | DistrictHighway, true, false, false},
        {"sl65", "Mercedes-Benz SL 65 AMG", 4, 75, DistrictDowntown | DistrictHighway, true, false, false},

        // Tier 5 — endgame / post-career exotics.
        {"fordgt", "Ford GT", 5, 75, DistrictHighway, true, false, false},
        {"murcielago", "Lamborghini Murcielago", 5, 70, DistrictDowntown | DistrictHighway, true, false, false},
        {"slr", "Mercedes-Benz SLR McLaren", 5, 65, DistrictDowntown | DistrictHighway, true, false, false},
        {"911Turbo", "Porsche 911 Turbo S", 5, 65, DistrictDowntown | DistrictHighway, true, false, false},
        {"carreragt", "Porsche Carrera GT", 5, 55, DistrictHighway, true, false, false},

        // Legendary/special pool. Never selected procedurally unless both
        // the progression system and explicit config allow it.
        {"911gt2", "Porsche 911 GT2", 5, 25, DistrictHighway, false, true, true},
        {"camaro", "Chevrolet Camaro SS", 5, 20, DistrictCamden, false, true, true},
        {"bmwm3", "BMW M3", 5, 18, DistrictDowntown | DistrictHighway, false, true, true},
        {"bmwm3gtr", "BMW M3 GTR", 5, 8, DistrictHighway, false, true, true},
        {"bmwm3gtre46", "BMW M3 GTR E46", 5, 5, DistrictHighway, false, true, true},
        {"corvettec6r", "Chevrolet Corvette C6.R", 5, 5, DistrictHighway, false, true, true}
    };

    return catalog;
}

std::optional<VehicleSelectionResult> selectVehicle(
    const std::vector<VehicleDefinition>& catalog,
    const VehicleSelectionContext& context
) {
    if (context.minimumTier > context.maximumTier) {
        return std::nullopt;
    }

    // First pass avoids repeating the exact previous vehicle.
    if (auto selected = selectPass(catalog, context, false)) {
        return selected;
    }

    // If that made the candidate set empty, allow the previous key.
    return selectPass(catalog, context, true);
}

} // namespace frr::domain
