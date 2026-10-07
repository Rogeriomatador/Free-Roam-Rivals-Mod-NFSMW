#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace frr::domain {

enum class UndergroundEntryState {
    Locked,
    Rumored,
    HuntAvailable,
    Discovered,
    ChallengeReady,
    Defeated
};

struct UndergroundRivalDefinition {
    int rank = 0;

    std::string rivalKey;
    std::string displayName;
    std::string districtHint;

    int minimumStreetRep = 0;
    int qualifierWinsRequired = 0;
    int minimumPinkSlipWins = 0;

    // Presentation metadata only. Empty means the UI should use a safe
    // placeholder/silhouette until custom art/audio is supplied.
    std::string portraitAsset;
    std::string introVoiceAsset;
    std::string defeatVoiceAsset;
    std::string themeAudioAsset;
};

struct UndergroundBlacklistProgress {
    bool careerCompleted = false;

    int streetRep = 0;
    int qualifierWinsCurrentRank = 0;
    int pinkSlipWins = 0;

    // Bit rank-1 represents ranks 1..32.
    std::uint32_t defeatedMask = 0;
    std::uint32_t discoveredMask = 0;

    // Ephemeral runtime fact: current target is physically live/near enough
    // for the encounter director to offer a challenge.
    bool currentTargetPresent = false;
};

struct UndergroundEntrySnapshot {
    int rank = 0;
    std::string rivalKey;
    std::string displayName;
    std::string districtHint;

    UndergroundEntryState state = UndergroundEntryState::Locked;

    bool identityRevealed = false;
    bool requirementsMet = false;
    bool worldSpawnEligible = false;
    bool challengeEligible = false;

    int streetRepRemaining = 0;
    int qualifierWinsRemaining = 0;
    int pinkSlipWinsRemaining = 0;

    std::string portraitAsset;
    std::string introVoiceAsset;
    std::string defeatVoiceAsset;
    std::string themeAudioAsset;
};

struct UndergroundBlacklistSnapshot {
    bool unlocked = false;
    bool completed = false;

    int currentRank = 0;

    bool currentTargetSpawnEligible = false;
    bool currentTargetChallengeEligible = false;

    std::vector<UndergroundEntrySnapshot> entries;
};

const std::vector<UndergroundRivalDefinition>&
defaultUndergroundBlacklist();

UndergroundBlacklistSnapshot evaluateUndergroundBlacklist(
    const std::vector<UndergroundRivalDefinition>& definitions,
    const UndergroundBlacklistProgress& progress
);

UndergroundBlacklistSnapshot evaluateUndergroundBlacklist(
    const UndergroundBlacklistProgress& progress
);

bool undergroundRankDefeated(
    std::uint32_t defeatedMask,
    int rank
);

bool undergroundRankDiscovered(
    std::uint32_t discoveredMask,
    int rank
);

std::uint32_t markUndergroundRankDefeated(
    std::uint32_t defeatedMask,
    int rank
);

std::uint32_t markUndergroundRankDiscovered(
    std::uint32_t discoveredMask,
    int rank
);

const char* undergroundEntryStateName(
    UndergroundEntryState state
);

} // namespace frr::domain
