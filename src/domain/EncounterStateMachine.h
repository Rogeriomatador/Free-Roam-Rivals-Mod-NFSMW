#pragma once

namespace frr::domain {

enum class EncounterState {
    Roaming,
    Interested,
    ChallengeAvailable,
    Accepted,
    Cooldown
};

struct EncounterTuning {
    float interestDistanceMeters = 90.0f;
    float challengeDistanceMeters = 20.0f;
    float interestConfirmSeconds = 0.75f;
    float challengeTimeoutSeconds = 12.0f;
    float cooldownSeconds = 45.0f;
};

struct EncounterInput {
    float deltaSeconds = 0.0f;
    float distanceMeters = 10000.0f;

    bool playerValid = false;
    bool rivalValid = false;
    bool acceptPressed = false;
};

class EncounterStateMachine {
public:
    explicit EncounterStateMachine(
        EncounterTuning tuning = {}
    );

    EncounterState state() const;
    float stateTimeSeconds() const;

    EncounterState tick(const EncounterInput& input);

    void markRaceFinished();
    void reset();

private:
    void transition(EncounterState next);

    EncounterTuning tuning_{};
    EncounterState state_ = EncounterState::Roaming;
    float stateTimeSeconds_ = 0.0f;
};

const char* encounterStateName(EncounterState state);

} // namespace frr::domain
