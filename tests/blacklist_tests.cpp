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

    UndergroundBlacklistProgress events{};

    UndergroundProgressEvent event{};
    event.kind =
        UndergroundProgressEventKind::TargetSighted;
    event.rank = 10;

    auto update =
        applyUndergroundProgressEvent(events, event);

    require(
        !update.applied &&
        update.rejectReason ==
            UndergroundProgressRejectReason::CareerNotCompleted,
        "progress events stay locked before career completion"
    );

    events.careerCompleted = true;

    update =
        applyUndergroundProgressEvent(events, event);

    require(
        update.applied &&
        undergroundRankDiscovered(
            update.progress.discoveredMask,
            10
        ),
        "sighting current eligible rank records discovery"
    );

    events = update.progress;

    event.kind =
        UndergroundProgressEventKind::TargetDefeated;

    update =
        applyUndergroundProgressEvent(events, event);

    require(
        update.applied &&
        undergroundRankDefeated(
            update.progress.defeatedMask,
            10
        ) &&
        update.previousRank == 10 &&
        update.currentRank == 9,
        "defeating current rank advances the ladder"
    );

    events = update.progress;

    event.kind =
        UndergroundProgressEventKind::TargetSighted;
    event.rank = 9;

    update =
        applyUndergroundProgressEvent(events, event);

    require(
        !update.applied &&
        update.rejectReason ==
            UndergroundProgressRejectReason::RequirementsNotMet,
        "next rank cannot be discovered before requirements"
    );

    event.kind =
        UndergroundProgressEventKind::StreetRepEarned;
    event.rank = 0;
    event.amount = 50;

    update =
        applyUndergroundProgressEvent(events, event);
    require(
        update.applied &&
        update.progress.streetRep == 50,
        "street rep event updates progress"
    );
    events = update.progress;

    event.kind =
        UndergroundProgressEventKind::QualifierWin;
    event.amount = 1;

    update =
        applyUndergroundProgressEvent(events, event);
    require(
        update.applied &&
        update.progress.qualifierWinsCurrentRank == 1,
        "qualifier win updates current-rank counter"
    );
    events = update.progress;

    event.kind =
        UndergroundProgressEventKind::TargetSighted;
    event.rank = 9;

    update =
        applyUndergroundProgressEvent(events, event);
    require(
        update.applied &&
        undergroundRankDiscovered(
            update.progress.discoveredMask,
            9
        ),
        "rank becomes discoverable after requirements"
    );
    events = update.progress;

    event.kind =
        UndergroundProgressEventKind::TargetDefeated;
    event.rank = 8;

    update =
        applyUndergroundProgressEvent(events, event);
    require(
        !update.applied &&
        update.rejectReason ==
            UndergroundProgressRejectReason::WrongTargetRank,
        "cannot defeat a future rank out of order"
    );

    event.kind =
        UndergroundProgressEventKind::TargetDefeated;
    event.rank = 9;

    update =
        applyUndergroundProgressEvent(events, event);
    require(
        update.applied &&
        update.progress.qualifierWinsCurrentRank == 0 &&
        update.currentRank == 8,
        "rank defeat resets qualifier wins for next rank"
    );
    events = update.progress;

    event.kind =
        UndergroundProgressEventKind::PinkSlipWin;
    event.rank = 0;
    event.amount = 2;

    update =
        applyUndergroundProgressEvent(events, event);
    require(
        update.applied &&
        update.progress.pinkSlipWins == 2,
        "pink-slip wins are tracked by progress events"
    );

    event.kind =
        UndergroundProgressEventKind::QualifierWin;
    event.amount = 0;

    update =
        applyUndergroundProgressEvent(events, event);
    require(
        !update.applied &&
        update.rejectReason ==
            UndergroundProgressRejectReason::InvalidAmount,
        "non-positive additive progress is rejected"
    );

    std::cout
        << "Free Roam Rivals underground blacklist tests passed.\n";
    return 0;
}
