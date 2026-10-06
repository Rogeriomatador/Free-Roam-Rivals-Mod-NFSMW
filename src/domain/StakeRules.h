#pragma once

#include <cstdint>
#include <string>

namespace frr::domain {

struct StakeContext {
    std::int64_t playerCash = 0;
    std::int64_t rivalCash = 0;

    int playerEligibleCars = 1;
    int rivalEligibleCars = 1;

    std::int64_t playerCarValue = 0;
    std::int64_t rivalCarValue = 0;

    bool supportedExecutable = false;
    bool pinkSlipFeatureVerified = false;
    bool safeGarageBridge = false;
    bool safeDestinationGarageSlot = false;
    bool transactionJournalAvailable = false;
    bool unsafeGameTransition = false;
};

struct StakeAvailability {
    bool practice = true;
    bool cash = false;
    bool pinkSlip = false;
    bool mixedCarAndCash = false;

    std::string cashReason;
    std::string pinkSlipReason;
};

StakeAvailability evaluateStakeAvailability(
    const StakeContext& context,
    std::int64_t requestedCashStake
);

} // namespace frr::domain
