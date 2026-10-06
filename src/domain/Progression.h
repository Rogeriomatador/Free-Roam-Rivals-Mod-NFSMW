#pragma once

namespace frr::domain {

enum class StreetRank {
    Unknown,
    Newcomer,
    Known,
    Respected,
    Notorious,
    Elite,
    Legend,
    Icon
};

struct ProgressionInput {
    bool careerCompleted = false;
    float careerProgress01 = 0.0f;
    int streetRep = 0;
    int totalWins = 0;
    int pinkSlipWins = 0;
};

struct ProgressionSnapshot {
    StreetRank rank = StreetRank::Unknown;
    int minimumRivalTier = 1;
    int maximumRivalTier = 2;
    int suggestedMaxActiveRivals = 1;
    bool rockportLegend = false;
    bool highStakesEligible = false;
    bool legendaryEligible = false;
};

StreetRank streetRankFromRep(int rep);
const char* streetRankName(StreetRank rank);
ProgressionSnapshot evaluateProgression(const ProgressionInput& input);

} // namespace frr::domain
