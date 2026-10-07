#include "RivalPopulation.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace frr::domain {
namespace {

std::uint64_t splitmix64(std::uint64_t x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

float unitFloat(std::uint64_t seed) {
    const std::uint64_t value = splitmix64(seed);
    const double normalized =
        static_cast<double>(value >> 11) /
        static_cast<double>(1ull << 53);

    return static_cast<float>(normalized);
}

float rangeFloat(
    std::uint64_t seed,
    float minimum,
    float maximum
) {
    return minimum +
        (maximum - minimum) * unitFloat(seed);
}

float clamp01(float value) {
    return std::clamp(value, 0.0f, 1.0f);
}

template <typename T, std::size_t N>
const T& choose(
    const std::array<T, N>& values,
    std::uint64_t seed
) {
    const std::size_t index =
        static_cast<std::size_t>(
            splitmix64(seed) % N
        );

    return values[index];
}

std::string generateName(std::uint64_t seed) {
    static constexpr std::array<const char*, 32> names = {
        "Alex", "Maya", "Marcus", "Diego",
        "Nina", "Tyler", "Chris", "Jordan",
        "Rico", "Zoe", "Dante", "Mia",
        "Jesse", "Ryan", "Lena", "Victor",
        "Noah", "Kara", "Leo", "Tess",
        "Evan", "Sofia", "Cole", "Jade",
        "Nate", "Lia", "Mason", "Ivy",
        "Derek", "Ava", "Rafael", "Skye"
    };

    return choose(names, seed ^ 0xA11CE5EEDull);
}

VisualArchetype generateVisualArchetype(
    std::uint64_t seed,
    int tier
) {
    // Lower tiers lean toward sleeper/OEM+/clean builds.
    // Higher tiers make aggressive/show-car styles more likely,
    // while remaining deterministic for a rival identity.
    const float roll = unitFloat(seed ^ 0x51574C45ull);

    if (tier <= 2) {
        if (roll < 0.28f) return VisualArchetype::Sleeper;
        if (roll < 0.55f) return VisualArchetype::OEMPlus;
        if (roll < 0.82f) return VisualArchetype::CleanStreet;
        return VisualArchetype::Tuner;
    }

    if (tier >= 5) {
        if (roll < 0.18f) return VisualArchetype::OEMPlus;
        if (roll < 0.42f) return VisualArchetype::CleanStreet;
        if (roll < 0.66f) return VisualArchetype::Aggressive;
        if (roll < 0.84f) return VisualArchetype::ShowCar;
        return VisualArchetype::Tuner;
    }

    if (roll < 0.16f) return VisualArchetype::Sleeper;
    if (roll < 0.34f) return VisualArchetype::OEMPlus;
    if (roll < 0.58f) return VisualArchetype::CleanStreet;
    if (roll < 0.82f) return VisualArchetype::Tuner;
    if (roll < 0.94f) return VisualArchetype::Aggressive;
    return VisualArchetype::ShowCar;
}

RivalChallengeStyle generateChallengeStyle(
    std::uint64_t seed,
    float aggression,
    float confidence
) {
    const float roll = unitFloat(seed ^ 0x4348414Cull);

    if (aggression > 0.72f && roll < 0.55f) {
        return RivalChallengeStyle::Provoker;
    }

    if (confidence > 0.72f && roll < 0.50f) {
        return RivalChallengeStyle::OvertakeAndSlow;
    }

    if (roll < 0.25f) {
        return RivalChallengeStyle::PullAheadAndWait;
    }

    if (roll < 0.60f) {
        return RivalChallengeStyle::MatchSpeed;
    }

    return RivalChallengeStyle::CleanInvite;
}

RivalPersonality generatePersonality(
    std::uint64_t seed,
    int tier,
    int streetRep
) {
    RivalPersonality out{};

    const float repPressure = std::clamp(
        static_cast<float>(streetRep) / 1000.0f,
        0.0f,
        1.0f
    );

    out.skill = clamp01(
        0.27f +
        static_cast<float>(tier) * 0.105f +
        repPressure * 0.08f +
        rangeFloat(seed ^ 0x534B494Cull, -0.08f, 0.08f)
    );

    out.aggression = clamp01(
        rangeFloat(seed ^ 0x41474752ull, 0.25f, 0.90f)
    );

    out.confidence = clamp01(
        0.30f +
        out.skill * 0.45f +
        rangeFloat(seed ^ 0x434F4E46ull, -0.12f, 0.18f)
    );

    out.riskTolerance = clamp01(
        0.20f +
        out.aggression * 0.45f +
        out.confidence * 0.25f +
        rangeFloat(seed ^ 0x5249534Bull, -0.10f, 0.10f)
    );

    out.policeFear = clamp01(
        0.78f -
        out.riskTolerance * 0.55f +
        rangeFloat(seed ^ 0x504F4C49ull, -0.12f, 0.12f)
    );

    return out;
}

std::int64_t generateStartingCash(
    std::uint64_t seed,
    int tier,
    float riskTolerance
) {
    const std::int64_t base =
        3500ll +
        static_cast<std::int64_t>(tier) * 8500ll;

    const std::int64_t variance =
        static_cast<std::int64_t>(
            rangeFloat(
                seed ^ 0x43415348ull,
                -2500.0f,
                7000.0f
            )
        );

    const std::int64_t riskBonus =
        static_cast<std::int64_t>(
            riskTolerance * 6500.0f
        );

    return std::max<std::int64_t>(
        1500ll,
        base + variance + riskBonus
    );
}

} // namespace

std::optional<GeneratedRival> generateProceduralRival(
    const ProceduralRivalRequest& request
) {
    VehicleSelectionContext vehicleContext{};
    vehicleContext.minimumTier = request.minimumTier;
    vehicleContext.maximumTier = request.maximumTier;
    vehicleContext.district = request.district;
    vehicleContext.allowProcedural = true;
    vehicleContext.allowLegendary =
        request.rockportLegend &&
        request.allowLegendary;
    vehicleContext.allowSpecial =
        request.rockportLegend &&
        request.allowSpecial;
    vehicleContext.seed =
        splitmix64(request.seed ^ 0x56454849434C45ull);
    vehicleContext.avoidKey = request.avoidVehicleKey;

    const auto& catalog = defaultVehicleCatalog();
    const auto selected = selectVehicle(
        catalog,
        vehicleContext
    );

    if (!selected) {
        return std::nullopt;
    }

    const VehicleDefinition& vehicle =
        catalog[selected->index];

    GeneratedRival out{};
    out.rivalId =
        splitmix64(request.seed ^ 0x524956414Cull);

    if (out.rivalId == 0) {
        out.rivalId = 1;
    }

    out.name = generateName(request.seed);
    out.origin = vehicle.legendary
        ? RivalOrigin::Legendary
        : RivalOrigin::ProceduralLocal;

    out.vehicleKey = std::string(vehicle.key);
    out.vehicleTier = vehicle.tier;

    out.personality = generatePersonality(
        request.seed,
        vehicle.tier,
        request.streetRep
    );

    out.visualArchetype = generateVisualArchetype(
        request.seed,
        vehicle.tier
    );

    out.challengeStyle = generateChallengeStyle(
        request.seed,
        out.personality.aggression,
        out.personality.confidence
    );

    out.startingCash = generateStartingCash(
        request.seed,
        vehicle.tier,
        out.personality.riskTolerance
    );

    out.visualSeed =
        splitmix64(request.seed ^ 0x56495355414Cull);
    out.performanceSeed =
        splitmix64(request.seed ^ 0x504552464F524Dull);

    out.persistent = false;

    return out;
}

void promoteToPersistent(GeneratedRival& rival) {
    rival.persistent = true;
}

PopulationBudget evaluatePopulationBudget(
    const PopulationBudgetInput& input
) {
    PopulationBudget out{};

    const float progress = std::clamp(
        input.careerProgress01,
        0.0f,
        1.0f
    );

    if (input.careerCompleted) {
        out.maxLiveRivals =
            input.streetRep >= 750 ? 3 : 2;
        out.maxProceduralLocals = 2;
        out.allowRevengeHunt = true;
        out.allowStreetMeet = input.streetRep >= 150;
        out.allowLegendarySightings =
            input.streetRep >= 500;
        return out;
    }

    out.maxLiveRivals = progress >= 0.60f ? 2 : 1;
    out.maxProceduralLocals = 1;

    out.allowRevengeHunt =
        progress >= 0.35f ||
        input.streetRep >= 100;

    out.allowStreetMeet =
        progress >= 0.50f &&
        input.streetRep >= 75;

    out.allowLegendarySightings = false;

    return out;
}

const char* visualArchetypeName(
    VisualArchetype archetype
) {
    switch (archetype) {
        case VisualArchetype::Sleeper:
            return "Sleeper";
        case VisualArchetype::CleanStreet:
            return "CleanStreet";
        case VisualArchetype::Tuner:
            return "Tuner";
        case VisualArchetype::Aggressive:
            return "Aggressive";
        case VisualArchetype::ShowCar:
            return "ShowCar";
        case VisualArchetype::OEMPlus:
            return "OEMPlus";
    }

    return "Unknown";
}

const char* challengeStyleName(
    RivalChallengeStyle style
) {
    switch (style) {
        case RivalChallengeStyle::OvertakeAndSlow:
            return "OvertakeAndSlow";
        case RivalChallengeStyle::MatchSpeed:
            return "MatchSpeed";
        case RivalChallengeStyle::PullAheadAndWait:
            return "PullAheadAndWait";
        case RivalChallengeStyle::Provoker:
            return "Provoker";
        case RivalChallengeStyle::CleanInvite:
            return "CleanInvite";
    }

    return "Unknown";
}

} // namespace frr::domain
