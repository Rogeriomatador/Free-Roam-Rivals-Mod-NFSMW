#include "Progression.h"

#include <algorithm>

namespace frr::domain {

StreetRank streetRankFromRep(int rep) {
    rep = std::max(rep, 0);

    if (rep >= 1000) return StreetRank::Icon;
    if (rep >= 750) return StreetRank::Legend;
    if (rep >= 500) return StreetRank::Elite;
    if (rep >= 300) return StreetRank::Notorious;
    if (rep >= 150) return StreetRank::Respected;
    if (rep >= 75) return StreetRank::Known;
    if (rep >= 25) return StreetRank::Newcomer;
    return StreetRank::Unknown;
}

const char* streetRankName(StreetRank rank) {
    switch (rank) {
        case StreetRank::Unknown: return "Unknown";
        case StreetRank::Newcomer: return "Newcomer";
        case StreetRank::Known: return "Known";
        case StreetRank::Respected: return "Respected";
        case StreetRank::Notorious: return "Notorious";
        case StreetRank::Elite: return "Elite";
        case StreetRank::Legend: return "Legend";
        case StreetRank::Icon: return "Icon";
    }

    return "Unknown";
}

ProgressionSnapshot evaluateProgression(const ProgressionInput& input) {
    ProgressionSnapshot out{};
    out.rank = streetRankFromRep(input.streetRep);
    out.rockportLegend = input.careerCompleted;

    const float progress = std::clamp(input.careerProgress01, 0.0f, 1.0f);

    if (input.careerCompleted) {
        out.minimumRivalTier = input.streetRep >= 500 ? 2 : 1;
        out.maximumRivalTier = 5;
    } else if (progress < 0.25f) {
        out.minimumRivalTier = 1;
        out.maximumRivalTier = 2;
    } else if (progress < 0.60f) {
        out.minimumRivalTier = 1;
        out.maximumRivalTier = 3;
    } else if (progress < 0.85f) {
        out.minimumRivalTier = 2;
        out.maximumRivalTier = 4;
    } else {
        out.minimumRivalTier = 2;
        out.maximumRivalTier = 5;
    }

    if (input.streetRep >= 300) {
        out.suggestedMaxActiveRivals = 2;
    }

    if (input.careerCompleted && input.streetRep >= 750) {
        out.suggestedMaxActiveRivals = 3;
    }

    out.highStakesEligible =
        input.streetRep >= 150 &&
        input.totalWins >= 10;

    out.legendaryEligible =
        input.careerCompleted &&
        input.streetRep >= 500 &&
        input.totalWins >= 30 &&
        input.pinkSlipWins >= 3;

    return out;
}

} // namespace frr::domain
