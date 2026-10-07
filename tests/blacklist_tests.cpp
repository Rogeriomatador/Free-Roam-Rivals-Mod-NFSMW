#include "domain/UndergroundBlacklist.h"

#include <cstdlib>
#include <iostream>

namespace {

void require(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace frr::domain;

    UndergroundBlacklistProgress progress{};

    auto snapshot = evaluateUndergroundBlacklist(progress);

    require(!snapshot.unlocked,
            "secondary blacklist stays locked before vanilla career completion");
    require(snapshot.currentRank == 0,
            "locked blacklist has no current target");

    progress.careerCompleted = true;
    snapshot = evaluateUndergroundBlacklist(progress);

    require(snapshot.unlocked,
            "career completion unlocks secondary blacklist");
    require(snapshot.currentRank == 10,
            "secondary blacklist starts at rank 10");
    require(snapshot.currentTargetSpawnEligible,
            "rank 10 may enter world hunt immediately");
    require(!snapshot.currentTargetChallengeEligible,
            "undiscovered target cannot be challenged immediately");

    progress.discoveredMask =
        markUndergroundRankDiscovered(
            progress.discoveredMask,
            10
        );

    snapshot = evaluateUndergroundBlacklist(progress);
    require(
        snapshot.entries.front().state ==
            UndergroundEntryState::Discovered,
        "sighted current rival becomes discovered"
    );

    progress.currentTargetPresent = true;
    snapshot = evaluateUndergroundBlacklist(progress);
    require(snapshot.currentTargetChallengeEligible,
            "present discovered current rival is challengeable");

    progress.currentTargetPresent = false;
    progress.defeatedMask =
        markUndergroundRankDefeated(
            progress.defeatedMask,
            10
        );
    progress.qualifierWinsCurrentRank = 0;

    snapshot = evaluateUndergroundBlacklist(progress);
    require(snapshot.currentRank == 9,
            "defeating rank 10 advances to rank 9");
    require(!snapshot.currentTargetSpawnEligible,
            "rank 9 remains a rumor before requirements");

    progress.streetRep = 50;
    progress.qualifierWinsCurrentRank = 1;
    snapshot = evaluateUndergroundBlacklist(progress);
    require(snapshot.currentTargetSpawnEligible,
            "rank 9 becomes huntable after requirements");

    for (int rank = 9; rank >= 1; --rank) {
        progress.defeatedMask =
            markUndergroundRankDefeated(
                progress.defeatedMask,
                rank
            );
    }

    snapshot = evaluateUndergroundBlacklist(progress);
    require(snapshot.completed,
            "defeating every rank completes secondary blacklist");
    require(snapshot.currentRank == 0,
            "completed ladder has no remaining target");

    require(
        markUndergroundRankDiscovered(0, 32) != 0,
        "rank masks support future expansion up to 32"
    );

    std::cout
        << "Free Roam Rivals underground blacklist tests passed.\n";
    return 0;
}
