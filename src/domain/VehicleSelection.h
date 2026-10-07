#pragma once

#include <cstdint>
#include <optional>
#include <string_view>
#include <vector>

namespace frr::domain {

enum DistrictMask : std::uint32_t {
    DistrictNone     = 0,
    DistrictRosewood = 1u << 0,
    DistrictCamden   = 1u << 1,
    DistrictDowntown = 1u << 2,
    DistrictHighway  = 1u << 3,
    DistrictAll      = DistrictRosewood |
                       DistrictCamden |
                       DistrictDowntown |
                       DistrictHighway
};

struct VehicleDefinition {
    std::string_view key;
    std::string_view displayName;

    int tier = 1;
    int baseWeight = 100;
    std::uint32_t preferredDistricts = DistrictAll;

    bool procedural = true;
    bool legendary = false;
    bool special = false;
};

struct VehicleSelectionContext {
    int minimumTier = 1;
    int maximumTier = 1;

    std::uint32_t district = DistrictNone;

    bool allowProcedural = true;
    bool allowLegendary = false;
    bool allowSpecial = false;

    // Deterministic seed. Once a generated rival becomes persistent,
    // its chosen vehicle key is stored and this selector is no longer
    // re-run for every sighting.
    std::uint64_t seed = 1;

    // Optional soft anti-repeat key. The selector prefers a different
    // vehicle when alternatives exist, but can fall back if necessary.
    std::string_view avoidKey;
};

struct VehicleSelectionResult {
    std::size_t index = 0;
    std::uint64_t roll = 0;
};

const std::vector<VehicleDefinition>& defaultVehicleCatalog();

std::optional<VehicleSelectionResult> selectVehicle(
    const std::vector<VehicleDefinition>& catalog,
    const VehicleSelectionContext& context
);

} // namespace frr::domain
