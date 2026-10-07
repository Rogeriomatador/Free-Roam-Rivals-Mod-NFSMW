#include "SpawnExperiment.h"

#include <algorithm>

namespace frr::domain {
namespace {

bool timeoutReached(float elapsed, float limit) {
    return limit > 0.0f && elapsed >= limit;
}

} // namespace

SpawnExperiment::SpawnExperiment(
    SpawnExperimentTuning tuning
) : tuning_(tuning) {}

void SpawnExperiment::begin() {
    state_ = SpawnExperimentState::AwaitConstruction;
    failure_ = SpawnExperimentFailure::None;
    pendingFailure_ = SpawnExperimentFailure::None;
    stateTimeSeconds_ = 0.0f;
    ownsVehicle_ = false;
    constructRequested_ = false;
    cleanupRequested_ = false;
    motionObserved_ = false;
    removalConfirmSamples_ = 0;
}

void SpawnExperiment::reset() {
    state_ = SpawnExperimentState::Idle;
    failure_ = SpawnExperimentFailure::None;
    pendingFailure_ = SpawnExperimentFailure::None;
    stateTimeSeconds_ = 0.0f;
    ownsVehicle_ = false;
    constructRequested_ = false;
    cleanupRequested_ = false;
    motionObserved_ = false;
    removalConfirmSamples_ = 0;
}

void SpawnExperiment::transition(
    SpawnExperimentState next
) {
    state_ = next;
    stateTimeSeconds_ = 0.0f;

    if (next == SpawnExperimentState::Cleanup) {
        cleanupRequested_ = false;
    }

    if (next == SpawnExperimentState::VerifyRemoval) {
        removalConfirmSamples_ = 0;
    }
}

void SpawnExperiment::requestFailure(
    SpawnExperimentFailure reason
) {
    if (state_ == SpawnExperimentState::Failed ||
        state_ == SpawnExperimentState::Succeeded) {
        return;
    }

    if (!ownsVehicle_) {
        failure_ = reason;
        transition(SpawnExperimentState::Failed);
        return;
    }

    if (pendingFailure_ == SpawnExperimentFailure::None) {
        pendingFailure_ = reason;
    }

    if (state_ != SpawnExperimentState::Cleanup &&
        state_ != SpawnExperimentState::VerifyRemoval) {
        transition(SpawnExperimentState::Cleanup);
    }
}

void SpawnExperiment::finishRemoval() {
    ownsVehicle_ = false;

    if (pendingFailure_ == SpawnExperimentFailure::None) {
        failure_ = SpawnExperimentFailure::None;
        transition(SpawnExperimentState::Succeeded);
    } else {
        failure_ = pendingFailure_;
        transition(SpawnExperimentState::Failed);
    }
}

SpawnExperimentUpdate SpawnExperiment::tick(
    const SpawnExperimentInput& input
) {
    const SpawnExperimentState before = state_;

    if (state_ == SpawnExperimentState::Idle ||
        state_ == SpawnExperimentState::Succeeded ||
        state_ == SpawnExperimentState::Failed) {
        return makeUpdate(before, false, false);
    }

    stateTimeSeconds_ +=
        std::max(input.deltaSeconds, 0.0f);

    if (!input.generationValid) {
        requestFailure(
            SpawnExperimentFailure::GenerationChanged
        );
    } else if (!input.worldSafe) {
        requestFailure(
            SpawnExperimentFailure::PreconditionsLost
        );
    } else if (
        state_ == SpawnExperimentState::AwaitConstruction &&
        !input.candidateValid
    ) {
        requestFailure(
            SpawnExperimentFailure::PreconditionsLost
        );
    }

    bool requestConstruct = false;
    bool requestCleanup = false;

    switch (state_) {
        case SpawnExperimentState::AwaitConstruction:
            if (!constructRequested_) {
                constructRequested_ = true;
                requestConstruct = true;
            }

            if (input.constructionFailed) {
                requestFailure(
                    SpawnExperimentFailure::ConstructionFailed
                );
            } else if (input.constructionSucceeded) {
                ownsVehicle_ = true;
                transition(
                    SpawnExperimentState::VerifyRegistries
                );
            } else if (timeoutReached(
                           stateTimeSeconds_,
                           tuning_.constructionTimeoutSeconds)) {
                requestFailure(
                    SpawnExperimentFailure::ConstructionTimeout
                );
            }
            break;

        case SpawnExperimentState::VerifyRegistries:
            if (input.pvehicleRegistered &&
                input.liveVehicleRegistered) {
                transition(
                    SpawnExperimentState::VerifyAI
                );
            } else if (timeoutReached(
                           stateTimeSeconds_,
                           tuning_.registryTimeoutSeconds)) {
                requestFailure(
                    SpawnExperimentFailure::RegistryTimeout
                );
            }
            break;

        case SpawnExperimentState::VerifyAI:
            if (input.aiAvailable) {
                transition(
                    SpawnExperimentState::ObserveMotion
                );
            } else if (timeoutReached(
                           stateTimeSeconds_,
                           tuning_.aiTimeoutSeconds)) {
                requestFailure(
                    SpawnExperimentFailure::AiTimeout
                );
            }
            break;

        case SpawnExperimentState::ObserveMotion:
            motionObserved_ =
                motionObserved_ ||
                input.nativeMotionObserved;

            if (motionObserved_ &&
                stateTimeSeconds_ >=
                    std::max(
                        tuning_.requiredMotionObservationSeconds,
                        0.0f)) {
                transition(
                    SpawnExperimentState::Cleanup
                );
            } else if (timeoutReached(
                           stateTimeSeconds_,
                           tuning_.motionTimeoutSeconds)) {
                requestFailure(
                    SpawnExperimentFailure::MotionTimeout
                );
            }
            break;

        case SpawnExperimentState::Cleanup:
            if (!cleanupRequested_) {
                cleanupRequested_ = true;
                requestCleanup = true;
                transition(
                    SpawnExperimentState::VerifyRemoval
                );
            }
            break;

        case SpawnExperimentState::VerifyRemoval:
            if (!input.pvehicleRegistered &&
                !input.liveVehicleRegistered) {
                if (removalConfirmSamples_ <
                    tuning_.removalConfirmSamples) {
                    ++removalConfirmSamples_;
                }
            } else {
                removalConfirmSamples_ = 0;
            }

            if (removalConfirmSamples_ >=
                std::max(
                    tuning_.removalConfirmSamples,
                    1u)) {
                finishRemoval();
            } else if (timeoutReached(
                           stateTimeSeconds_,
                           tuning_.cleanupTimeoutSeconds)) {
                failure_ =
                    SpawnExperimentFailure::CleanupTimeout;
                transition(
                    SpawnExperimentState::Failed
                );
            }
            break;

        case SpawnExperimentState::Idle:
        case SpawnExperimentState::Succeeded:
        case SpawnExperimentState::Failed:
            break;
    }

    return makeUpdate(
        before,
        requestConstruct,
        requestCleanup
    );
}

SpawnExperimentState SpawnExperiment::state() const {
    return state_;
}

SpawnExperimentFailure SpawnExperiment::failure() const {
    return failure_;
}

bool SpawnExperiment::ownsVehicle() const {
    return ownsVehicle_;
}

float SpawnExperiment::stateTimeSeconds() const {
    return stateTimeSeconds_;
}

SpawnExperimentUpdate SpawnExperiment::makeUpdate(
    SpawnExperimentState before,
    bool requestConstruct,
    bool requestCleanup
) const {
    SpawnExperimentUpdate out{};
    out.state = state_;
    out.failure = failure_;
    out.stateChanged = before != state_;
    out.requestConstruct = requestConstruct;
    out.requestCleanup = requestCleanup;
    out.ownsVehicle = ownsVehicle_;
    out.lifecycleProven =
        state_ == SpawnExperimentState::Succeeded;
    out.disableSpawningForSession =
        state_ == SpawnExperimentState::Failed;
    return out;
}

const char* spawnExperimentStateName(
    SpawnExperimentState state
) {
    switch (state) {
        case SpawnExperimentState::Idle:
            return "Idle";
        case SpawnExperimentState::AwaitConstruction:
            return "AwaitConstruction";
        case SpawnExperimentState::VerifyRegistries:
            return "VerifyRegistries";
        case SpawnExperimentState::VerifyAI:
            return "VerifyAI";
        case SpawnExperimentState::ObserveMotion:
            return "ObserveMotion";
        case SpawnExperimentState::Cleanup:
            return "Cleanup";
        case SpawnExperimentState::VerifyRemoval:
            return "VerifyRemoval";
        case SpawnExperimentState::Succeeded:
            return "Succeeded";
        case SpawnExperimentState::Failed:
            return "Failed";
    }

    return "Unknown";
}

const char* spawnExperimentFailureName(
    SpawnExperimentFailure failure
) {
    switch (failure) {
        case SpawnExperimentFailure::None:
            return "None";
        case SpawnExperimentFailure::PreconditionsLost:
            return "PreconditionsLost";
        case SpawnExperimentFailure::GenerationChanged:
            return "GenerationChanged";
        case SpawnExperimentFailure::ConstructionFailed:
            return "ConstructionFailed";
        case SpawnExperimentFailure::ConstructionTimeout:
            return "ConstructionTimeout";
        case SpawnExperimentFailure::RegistryTimeout:
            return "RegistryTimeout";
        case SpawnExperimentFailure::AiTimeout:
            return "AiTimeout";
        case SpawnExperimentFailure::MotionTimeout:
            return "MotionTimeout";
        case SpawnExperimentFailure::CleanupTimeout:
            return "CleanupTimeout";
    }

    return "Unknown";
}

} // namespace frr::domain
