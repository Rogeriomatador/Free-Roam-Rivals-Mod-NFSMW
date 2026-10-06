#include "StakeRules.h"

#include <algorithm>
#include <cstdlib>

namespace frr::domain {

StakeAvailability evaluateStakeAvailability(
    const StakeContext& context,
    std::int64_t requestedCashStake
) {
    StakeAvailability out{};

    const std::int64_t cashStake = std::max<std::int64_t>(requestedCashStake, 0);

    if (context.unsafeGameTransition) {
        out.cashReason = "Unsafe game transition.";
    } else if (context.playerCash < cashStake) {
        out.cashReason = "Player has insufficient cash.";
    } else if (context.rivalCash < cashStake) {
        out.cashReason = "Rival has insufficient cash.";
    } else {
        out.cash = true;
        out.cashReason = "Available.";
    }

    if (!context.supportedExecutable) {
        out.pinkSlipReason = "Unsupported executable.";
        return out;
    }

    if (!context.pinkSlipFeatureVerified) {
        out.pinkSlipReason = "Pink-slip transfer path is not verified.";
        return out;
    }

    if (!context.safeGarageBridge) {
        out.pinkSlipReason = "Garage bridge is not verified.";
        return out;
    }

    if (context.unsafeGameTransition) {
        out.pinkSlipReason = "Unsafe game transition.";
        return out;
    }

    if (context.playerEligibleCars < 2) {
        out.pinkSlipReason = "Player must own at least two eligible cars.";
        return out;
    }

    if (context.rivalEligibleCars < 2) {
        out.pinkSlipReason = "Rival must own at least two eligible cars.";
        return out;
    }

    if (!context.safeDestinationGarageSlot) {
        out.pinkSlipReason = "No verified safe destination garage slot.";
        return out;
    }

    if (!context.transactionJournalAvailable) {
        out.pinkSlipReason = "Transaction journal is unavailable.";
        return out;
    }

    out.pinkSlip = true;
    out.pinkSlipReason = "Available.";

    const std::int64_t delta =
        context.playerCarValue > context.rivalCarValue
            ? context.playerCarValue - context.rivalCarValue
            : context.rivalCarValue - context.playerCarValue;

    if (delta == 0) {
        out.mixedCarAndCash = true;
    } else if (context.playerCarValue > context.rivalCarValue) {
        out.mixedCarAndCash = context.rivalCash >= delta;
    } else {
        out.mixedCarAndCash = context.playerCash >= delta;
    }

    return out;
}

} // namespace frr::domain
