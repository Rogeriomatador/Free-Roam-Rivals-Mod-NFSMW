#pragma once

namespace frr::domain {

enum class OutrunOutcome {
    InProgress,
    PlayerWon,
    RivalWon,
    Draw,
    Aborted
};

enum class OutrunLeader {
    Tied,
    Player,
    Rival
};

struct OutrunTuning {
    float winLeadMeters = 300.0f;
    float leadHoldSeconds = 3.0f;
    float maxDurationSeconds = 300.0f;
    float timeoutTieMeters = 5.0f;
};

struct OutrunInput {
    float deltaSeconds = 0.0f;

    // Positive means the player is leading, negative means the rival is
    // leading. The runtime adapter can later derive this from road progress.
    float signedLeadMeters = 0.0f;

    bool playerValid = true;
    bool rivalValid = true;
    bool unsafeTransition = false;
    bool cancelRequested = false;
};

class OutrunRace {
public:
    explicit OutrunRace(OutrunTuning tuning = {});

    void begin();
    OutrunOutcome tick(const OutrunInput& input);
    void abort();
    void reset();

    bool running() const;
    OutrunOutcome outcome() const;
    OutrunLeader leader() const;

    float elapsedSeconds() const;
    float signedLeadMeters() const;
    float holdSeconds() const;
    float holdProgress01() const;

private:
    OutrunTuning tuning_{};
    OutrunOutcome outcome_ = OutrunOutcome::InProgress;
    bool running_ = false;
    float elapsedSeconds_ = 0.0f;
    float signedLeadMeters_ = 0.0f;
    float holdSeconds_ = 0.0f;
    OutrunLeader thresholdLeader_ = OutrunLeader::Tied;
};

const char* outrunOutcomeName(OutrunOutcome outcome);
const char* outrunLeaderName(OutrunLeader leader);

} // namespace frr::domain
