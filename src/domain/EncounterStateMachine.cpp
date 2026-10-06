#include "EncounterStateMachine.h"

#include <algorithm>

namespace frr::domain {

EncounterStateMachine::EncounterStateMachine(
    EncounterTuning tuning
) : tuning_(tuning) {}

EncounterState EncounterStateMachine::state() const {
    return state_;
}

float EncounterStateMachine::stateTimeSeconds() const {
    return stateTimeSeconds_;
}

void EncounterStateMachine::transition(
    EncounterState next
) {
    state_ = next;
    stateTimeSeconds_ = 0.0f;
}

void EncounterStateMachine::reset() {
    transition(EncounterState::Roaming);
}

void EncounterStateMachine::markRaceFinished() {
    transition(EncounterState::Cooldown);
}

EncounterState EncounterStateMachine::tick(
    const EncounterInput& input
) {
    const float dt = std::max(input.deltaSeconds, 0.0f);
    stateTimeSeconds_ += dt;

    if (!input.playerValid || !input.rivalValid) {
        reset();
        return state_;
    }

    switch (state_) {
        case EncounterState::Roaming:
            if (input.distanceMeters <=
                tuning_.interestDistanceMeters) {
                transition(EncounterState::Interested);
            }
            break;

        case EncounterState::Interested:
            if (input.distanceMeters >
                tuning_.interestDistanceMeters * 1.25f) {
                transition(EncounterState::Roaming);
                break;
            }

            if (stateTimeSeconds_ >=
                    tuning_.interestConfirmSeconds &&
                input.distanceMeters <=
                    tuning_.challengeDistanceMeters) {
                transition(
                    EncounterState::ChallengeAvailable
                );
            }
            break;

        case EncounterState::ChallengeAvailable:
            if (input.acceptPressed) {
                transition(EncounterState::Accepted);
                break;
            }

            if (input.distanceMeters >
                    tuning_.interestDistanceMeters ||
                stateTimeSeconds_ >=
                    tuning_.challengeTimeoutSeconds) {
                transition(EncounterState::Roaming);
            }
            break;

        case EncounterState::Accepted:
            break;

        case EncounterState::Cooldown:
            if (stateTimeSeconds_ >= tuning_.cooldownSeconds) {
                transition(EncounterState::Roaming);
            }
            break;
    }

    return state_;
}

const char* encounterStateName(EncounterState state) {
    switch (state) {
        case EncounterState::Roaming:
            return "Roaming";
        case EncounterState::Interested:
            return "Interested";
        case EncounterState::ChallengeAvailable:
            return "ChallengeAvailable";
        case EncounterState::Accepted:
            return "Accepted";
        case EncounterState::Cooldown:
            return "Cooldown";
    }

    return "Unknown";
}

} // namespace frr::domain
