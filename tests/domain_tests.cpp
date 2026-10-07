#include "domain/EncounterStateMachine.h"
#include "domain/Progression.h"
#include "domain/StakeRules.h"
#include "domain/VehicleSelection.h"

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

    EncounterTuning tuning{};
    tuning.interestDistanceMeters = 90.0f;
    tuning.challengeDistanceMeters = 20.0f;
    tuning.interestConfirmSeconds = 0.5f;
    tuning.challengeTimeoutSeconds = 2.0f;
    tuning.cooldownSeconds = 1.0f;

    EncounterStateMachine encounter(tuning);

    EncounterInput input{};
    input.playerValid = true;
    input.rivalValid = true;
    input.distanceMeters = 50.0f;
    input.deltaSeconds = 0.1f;

    require(
        encounter.tick(input) == EncounterState::Interested,
        "nearby rival enters Interested"
    );

    input.distanceMeters = 15.0f;
    input.deltaSeconds = 0.6f;

    require(
        encounter.tick(input) == EncounterState::ChallengeAvailable,
        "close confirmed rival becomes challengeable"
    );

    input.acceptPressed = true;
    input.deltaSeconds = 0.01f;

    require(
        encounter.tick(input) == EncounterState::Accepted,
        "accept input starts encounter"
    );

    encounter.markRaceFinished();
    require(
        encounter.state() == EncounterState::Cooldown,
        "race result starts cooldown"
    );

    input.acceptPressed = false;
    input.deltaSeconds = 1.1f;

    require(
        encounter.tick(input) == EncounterState::Roaming,
        "cooldown returns to roaming"
    );

    const auto& catalog = defaultVehicleCatalog();
    require(catalog.size() >= 30, "default vehicle catalog populated");

    VehicleSelectionContext earlyCars{};
    earlyCars.minimumTier = 1;
    earlyCars.maximumTier = 2;
    earlyCars.district = DistrictRosewood;
    earlyCars.seed = 12345;

    const auto earlyCar = selectVehicle(catalog, earlyCars);
    require(earlyCar.has_value(), "early vehicle selection exists");
    require(
        catalog[earlyCar->index].tier >= 1 &&
        catalog[earlyCar->index].tier <= 2,
        "early selection obeys tier ceiling"
    );
    require(
        !catalog[earlyCar->index].legendary &&
        !catalog[earlyCar->index].special,
        "ordinary early selection excludes special cars"
    );

    VehicleSelectionContext legendaryBlocked{};
    legendaryBlocked.minimumTier = 5;
    legendaryBlocked.maximumTier = 5;
    legendaryBlocked.allowLegendary = false;
    legendaryBlocked.allowSpecial = false;
    legendaryBlocked.seed = 998877;

    for (int i = 0; i < 200; ++i) {
        legendaryBlocked.seed =
            998877ull + static_cast<std::uint64_t>(i);

        const auto picked =
            selectVehicle(catalog, legendaryBlocked);

        require(
            picked.has_value(),
            "tier-5 normal selection exists"
        );

        require(
            !catalog[picked->index].legendary &&
            !catalog[picked->index].special,
            "legendary/special excluded unless explicitly allowed"
        );
    }

    VehicleSelectionContext deterministic{};
    deterministic.minimumTier = 1;
    deterministic.maximumTier = 5;
    deterministic.district = DistrictCamden;
    deterministic.seed = 424242;

    const auto first = selectVehicle(catalog, deterministic);
    const auto second = selectVehicle(catalog, deterministic);

    require(first.has_value() && second.has_value(),
            "deterministic selection exists");
    require(first->index == second->index,
            "same seed gives same vehicle");

    VehicleSelectionContext noImmediateRepeat = deterministic;
    noImmediateRepeat.avoidKey =
        catalog[first->index].key;

    const auto different =
        selectVehicle(catalog, noImmediateRepeat);

    require(different.has_value(),
            "anti-repeat selection exists");
    require(
        catalog[different->index].key !=
            catalog[first->index].key,
        "soft anti-repeat avoids previous model when alternatives exist"
    );

    VehicleSelectionContext legend{};
    legend.minimumTier = 5;
    legend.maximumTier = 5;
    legend.allowLegendary = true;
    legend.allowSpecial = true;

    bool sawLegendary = false;
    for (std::uint64_t seed = 1; seed < 5000; ++seed) {
        legend.seed = seed;
        const auto picked = selectVehicle(catalog, legend);
        if (picked &&
            catalog[picked->index].legendary) {
            sawLegendary = true;
            break;
        }
    }

    require(sawLegendary,
            "legendary pool becomes reachable when explicitly enabled");

    std::cout << "Free Roam Rivals domain tests passed.\n";
    return 0;
}
