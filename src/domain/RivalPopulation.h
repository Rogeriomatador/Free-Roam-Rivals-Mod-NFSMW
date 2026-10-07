#pragma once

#include "VehicleSelection.h"

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace frr::domain {

enum class RivalOrigin {
    Authored,
    ProceduralLocal,
    Legendary,
    HistoryReturn
};

enum class VisualArchetype {
    Sleeper,
    CleanStreet,
    Tuner,
    Aggressive,
    ShowCar,
    OEMPlus
};

enum class RivalChallengeStyle {
    OvertakeAndSlow,
    MatchSpeed,
    PullAheadAndWait,
    Provoker,
    CleanInvite
};

struct RivalPersonality {
    float skill = 0.5f;
    float aggression = 0.5f;
    float confidence = 0.5f;
    float riskTolerance = 0.5f;
    float policeFear = 0.5f;
};

struct ProceduralRivalRequest {
    std::uint64_t seed = 1;

    int minimumTier = 1;
    int maximumTier = 1;

    std::uint32_t district = DistrictNone;

    int streetRep = 0;
    bool rockportLegend = false;
    bool allowLegendary = false;
    bool allowSpecial = false;

    std::string_view avoidVehicleKey;
};

struct GeneratedRival {
    std::uint64_t rivalId = 0;
    std::string name;

    RivalOrigin origin = RivalOrigin::ProceduralLocal;
    VisualArchetype visualArchetype = VisualArchetype::CleanStreet;
    RivalChallengeStyle challengeStyle = RivalChallengeStyle::MatchSpeed;
    RivalPersonality personality{};

    std::string vehicleKey;
    int vehicleTier = 1;

    std::int64_t startingCash = 0;

    std::uint64_t visualSeed = 0;
    std::uint64_t performanceSeed = 0;

    bool persistent = false;
};

struct PopulationBudgetInput {
    float careerProgress01 = 0.0f;
    bool careerCompleted = false;
    int streetRep = 0;
};

struct PopulationBudget {
    int maxLiveRivals = 1;
    int maxProceduralLocals = 1;
    bool allowRevengeHunt = false;
    bool allowStreetMeet = false;
    bool allowLegendarySightings = false;
};

std::optional<GeneratedRival> generateProceduralRival(
    const ProceduralRivalRequest& request
);

void promoteToPersistent(GeneratedRival& rival);

PopulationBudget evaluatePopulationBudget(
    const PopulationBudgetInput& input
);

const char* visualArchetypeName(VisualArchetype archetype);
const char* challengeStyleName(RivalChallengeStyle style);

} // namespace frr::domain
