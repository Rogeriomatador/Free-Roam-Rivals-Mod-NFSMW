#include "domain/Progression.h"
#include "domain/StakeRules.h"

#include <cstdlib>
#include <iostream>

namespace {

void require(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAILED: " << message << "\n";
        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace frr::domain;

    require(streetRankFromRep(0) == StreetRank::Unknown, "0 rep rank");
    require(streetRankFromRep(25) == StreetRank::Newcomer, "25 rep rank");
    require(streetRankFromRep(1000) == StreetRank::Icon, "1000 rep rank");

    ProgressionInput early{};
    early.careerProgress01 = 0.10f;
    early.streetRep = 80;

    const auto earlySnapshot = evaluateProgression(early);
    require(earlySnapshot.maximumRivalTier == 2, "early career tier cap");
    require(!earlySnapshot.rockportLegend, "early career is not endgame");

    ProgressionInput endgame{};
    endgame.careerCompleted = true;
    endgame.careerProgress01 = 1.0f;
    endgame.streetRep = 800;
    endgame.totalWins = 40;
    endgame.pinkSlipWins = 5;

    const auto endgameSnapshot = evaluateProgression(endgame);
    require(endgameSnapshot.rockportLegend, "career complete enables Rockport Legend");
    require(endgameSnapshot.maximumRivalTier == 5, "endgame uses top tier");
    require(endgameSnapshot.legendaryEligible, "legendary eligibility");

    StakeContext unsafe{};
    unsafe.supportedExecutable = true;
    unsafe.pinkSlipFeatureVerified = true;
    unsafe.safeGarageBridge = true;
    unsafe.safeDestinationGarageSlot = true;
    unsafe.transactionJournalAvailable = true;
    unsafe.playerEligibleCars = 1;
    unsafe.rivalEligibleCars = 3;

    auto stake = evaluateStakeAvailability(unsafe, 0);
    require(!stake.pinkSlip, "last player car must be protected");

    StakeContext valid{};
    valid.playerCash = 50000;
    valid.rivalCash = 50000;
    valid.playerEligibleCars = 3;
    valid.rivalEligibleCars = 3;
    valid.playerCarValue = 100000;
    valid.rivalCarValue = 80000;
    valid.supportedExecutable = true;
    valid.pinkSlipFeatureVerified = true;
    valid.safeGarageBridge = true;
    valid.safeDestinationGarageSlot = true;
    valid.transactionJournalAvailable = true;

    stake = evaluateStakeAvailability(valid, 5000);
    require(stake.cash, "cash stake should be available");
    require(stake.pinkSlip, "pink slip should be available");
    require(stake.mixedCarAndCash, "rival can cover mixed-stake delta");

    std::cout << "Free Roam Rivals domain tests passed.\n";
    return 0;
}
