#include "UndergroundBlacklist.h"

#include <algorithm>

namespace frr::domain {
namespace {

std::uint32_t rankBit(int rank) {
    if (rank < 1 || rank > 32) {
        return 0;
    }

    return 1u << static_cast<unsigned>(rank - 1);
}

int highestRank(const std::vector<UndergroundRivalDefinition>& definitions) {
    int result = 0;

    for (const auto& entry : definitions) {
        result = std::max(result, entry.rank);
    }

    return result;
}

int lowestRank(const std::vector<UndergroundRivalDefinition>& definitions) {
    int result = 33;

    for (const auto& entry : definitions) {
        if (entry.rank >= 1) {
            result = std::min(result, entry.rank);
        }
    }

    return result == 33 ? 0 : result;
}

int findCurrentRank(
    const std::vector<UndergroundRivalDefinition>& definitions,
    std::uint32_t defeatedMask
) {
    const int first = highestRank(definitions);
    const int last = lowestRank(definitions);

    if (first == 0 || last == 0) {
        return 0;
    }

    for (int rank = first; rank >= last; --rank) {
        bool defined = false;

        for (const auto& entry : definitions) {
            if (entry.rank == rank) {
                defined = true;
                break;
            }
        }

        if (!defined) {
            continue;
        }

        if (!undergroundRankDefeated(defeatedMask, rank)) {
            return rank;
        }
    }

    return 0;
}

bool requirementsMet(
    const UndergroundRivalDefinition& definition,
    const UndergroundBlacklistProgress& progress
) {
    return
        progress.streetRep >= definition.minimumStreetRep &&
        progress.qualifierWinsCurrentRank >=
            definition.qualifierWinsRequired &&
        progress.pinkSlipWins >= definition.minimumPinkSlipWins;
}

} // namespace

const std::vector<UndergroundRivalDefinition>&
defaultUndergroundBlacklist() {
    // Names/character art are intentionally provisional. Internal rivalKey
    // values are the stable identity; portraits and voice assets can be
    // supplied later without rewriting progression data.
    static const std::vector<UndergroundRivalDefinition> definitions = {
        {10, "ub10", "Diego",  "Rosewood",  0,   0, 0, "", "", "", ""},
        { 9, "ub09", "Maya",   "Downtown", 50,   1, 0, "", "", "", ""},
        { 8, "ub08", "Marcos", "Camden",   100,  1, 0, "", "", "", ""},
        { 7, "ub07", "Nina",   "Rosewood", 150,  2, 0, "", "", "", ""},
        { 6, "ub06", "Victor", "Camden",   220,  2, 0, "", "", "", ""},
        { 5, "ub05", "Jade",   "Downtown", 300,  2, 0, "", "", "", ""},
        { 4, "ub04", "Rafael", "Camden",   400,  3, 0, "", "", "", ""},
        { 3, "ub03", "Skye",   "Downtown", 520,  3, 0, "", "", "", ""},
        { 2, "ub02", "Dante",  "Rockport", 650,  4, 0, "", "", "", ""},
        { 1, "ub01", "Ghost",  "Rockport", 800,  5, 0, "", "", "", ""}
    };

    return definitions;
}

UndergroundBlacklistSnapshot evaluateUndergroundBlacklist(
    const std::vector<UndergroundRivalDefinition>& definitions,
    const UndergroundBlacklistProgress& progress
) {
    UndergroundBlacklistSnapshot out{};
    out.unlocked = progress.careerCompleted;

    const int currentRank = out.unlocked
        ? findCurrentRank(definitions, progress.defeatedMask)
        : 0;

    out.currentRank = currentRank;
    out.completed = out.unlocked && currentRank == 0 &&
        !definitions.empty();

    out.entries.reserve(definitions.size());

    for (const auto& definition : definitions) {
        UndergroundEntrySnapshot entry{};
        entry.rank = definition.rank;
        entry.rivalKey = definition.rivalKey;
        entry.displayName = definition.displayName;
        entry.districtHint = definition.districtHint;
        entry.portraitAsset = definition.portraitAsset;
        entry.introVoiceAsset = definition.introVoiceAsset;
        entry.defeatVoiceAsset = definition.defeatVoiceAsset;
        entry.themeAudioAsset = definition.themeAudioAsset;

        const bool defeated = undergroundRankDefeated(
            progress.defeatedMask,
            definition.rank
        );

        const bool discovered = undergroundRankDiscovered(
            progress.discoveredMask,
            definition.rank
        );

        entry.identityRevealed = discovered || defeated;

        entry.streetRepRemaining = std::max(
            0,
            definition.minimumStreetRep - progress.streetRep
        );

        entry.qualifierWinsRemaining = std::max(
            0,
            definition.qualifierWinsRequired -
                progress.qualifierWinsCurrentRank
        );

        entry.pinkSlipWinsRemaining = std::max(
            0,
            definition.minimumPinkSlipWins -
                progress.pinkSlipWins
        );

        entry.requirementsMet = requirementsMet(
            definition,
            progress
        );

        if (!out.unlocked) {
            entry.state = UndergroundEntryState::Locked;
        } else if (defeated) {
            entry.state = UndergroundEntryState::Defeated;
        } else if (definition.rank != currentRank) {
            entry.state = UndergroundEntryState::Locked;
        } else if (!entry.requirementsMet) {
            entry.state = UndergroundEntryState::Rumored;
        } else if (!discovered) {
            entry.state = UndergroundEntryState::HuntAvailable;
            entry.worldSpawnEligible = true;
        } else if (progress.currentTargetPresent) {
            entry.state = UndergroundEntryState::ChallengeReady;
            entry.worldSpawnEligible = true;
            entry.challengeEligible = true;
        } else {
            entry.state = UndergroundEntryState::Discovered;
            entry.worldSpawnEligible = true;
        }

        if (definition.rank == currentRank) {
            out.currentTargetSpawnEligible =
                entry.worldSpawnEligible;
            out.currentTargetChallengeEligible =
                entry.challengeEligible;
        }

        out.entries.push_back(std::move(entry));
    }

    std::sort(
        out.entries.begin(),
        out.entries.end(),
        [](const auto& a, const auto& b) {
            return a.rank > b.rank;
        }
    );

    return out;
}

UndergroundBlacklistSnapshot evaluateUndergroundBlacklist(
    const UndergroundBlacklistProgress& progress
) {
    return evaluateUndergroundBlacklist(
        defaultUndergroundBlacklist(),
        progress
    );
}

bool undergroundRankDefeated(
    std::uint32_t defeatedMask,
    int rank
) {
    const std::uint32_t bit = rankBit(rank);
    return bit != 0 && (defeatedMask & bit) != 0;
}

bool undergroundRankDiscovered(
    std::uint32_t discoveredMask,
    int rank
) {
    const std::uint32_t bit = rankBit(rank);
    return bit != 0 && (discoveredMask & bit) != 0;
}

std::uint32_t markUndergroundRankDefeated(
    std::uint32_t defeatedMask,
    int rank
) {
    return defeatedMask | rankBit(rank);
}

std::uint32_t markUndergroundRankDiscovered(
    std::uint32_t discoveredMask,
    int rank
) {
    return discoveredMask | rankBit(rank);
}

const char* undergroundEntryStateName(
    UndergroundEntryState state
) {
    switch (state) {
        case UndergroundEntryState::Locked:
            return "Locked";
        case UndergroundEntryState::Rumored:
            return "Rumored";
        case UndergroundEntryState::HuntAvailable:
            return "HuntAvailable";
        case UndergroundEntryState::Discovered:
            return "Discovered";
        case UndergroundEntryState::ChallengeReady:
            return "ChallengeReady";
        case UndergroundEntryState::Defeated:
            return "Defeated";
    }

    return "Unknown";
}

} // namespace frr::domain
