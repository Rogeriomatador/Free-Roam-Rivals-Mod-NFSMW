#include "OutrunRace.h"

#include <algorithm>

namespace frr::domain {

OutrunRace::OutrunRace(OutrunTuning tuning)
    : tuning_(tuning) {}

void OutrunRace::begin() {
    running_ = true;
    outcome_ = OutrunOutcome::InProgress;
    elapsedSeconds_ = 0.0f;
    signedLeadMeters_ = 0.0f;
    holdSeconds_ = 0.0f;
    thresholdLeader_ = OutrunLeader::Tied;
}

OutrunOutcome OutrunRace::tick(const OutrunInput& input) {
    if (!running_) {
        return outcome_;
    }

    if (!input.playerValid ||
        !input.rivalValid ||
        input.unsafeTransition ||
        input.cancelRequested) {
        abort();
        return outcome_;
    }

    const float dt = std::max(input.deltaSeconds, 0.0f);
    elapsedSeconds_ += dt;
    signedLeadMeters_ = input.signedLeadMeters;

    const float winLead = std::max(tuning_.winLeadMeters, 0.0f);

    OutrunLeader thresholdLeader = OutrunLeader::Tied;
    if (signedLeadMeters_ >= winLead) {
        thresholdLeader = OutrunLeader::Player;
    } else if (signedLeadMeters_ <= -winLead) {
        thresholdLeader = OutrunLeader::Rival;
    }

    if (thresholdLeader == OutrunLeader::Tied) {
        holdSeconds_ = 0.0f;
        thresholdLeader_ = OutrunLeader::Tied;
    } else {
        if (thresholdLeader_ != thresholdLeader) {
            holdSeconds_ = 0.0f;
        }

        thresholdLeader_ = thresholdLeader;
        holdSeconds_ += dt;

        if (holdSeconds_ >=
            std::max(tuning_.leadHoldSeconds, 0.0f)) {
            outcome_ = thresholdLeader == OutrunLeader::Player
                ? OutrunOutcome::PlayerWon
                : OutrunOutcome::RivalWon;
            running_ = false;
            return outcome_;
        }
    }

    if (tuning_.maxDurationSeconds > 0.0f &&
        elapsedSeconds_ >= tuning_.maxDurationSeconds) {
        const float tieBand =
            std::max(tuning_.timeoutTieMeters, 0.0f);

        if (signedLeadMeters_ > tieBand) {
            outcome_ = OutrunOutcome::PlayerWon;
        } else if (signedLeadMeters_ < -tieBand) {
            outcome_ = OutrunOutcome::RivalWon;
        } else {
            outcome_ = OutrunOutcome::Draw;
        }

        running_ = false;
    }

    return outcome_;
}

void OutrunRace::abort() {
    outcome_ = OutrunOutcome::Aborted;
    running_ = false;
    holdSeconds_ = 0.0f;
    thresholdLeader_ = OutrunLeader::Tied;
}

void OutrunRace::reset() {
    running_ = false;
    outcome_ = OutrunOutcome::InProgress;
    elapsedSeconds_ = 0.0f;
    signedLeadMeters_ = 0.0f;
    holdSeconds_ = 0.0f;
    thresholdLeader_ = OutrunLeader::Tied;
}

bool OutrunRace::running() const {
    return running_;
}

OutrunOutcome OutrunRace::outcome() const {
    return outcome_;
}

OutrunLeader OutrunRace::leader() const {
    if (signedLeadMeters_ > 0.0f) {
        return OutrunLeader::Player;
    }

    if (signedLeadMeters_ < 0.0f) {
        return OutrunLeader::Rival;
    }

    return OutrunLeader::Tied;
}

float OutrunRace::elapsedSeconds() const {
    return elapsedSeconds_;
}

float OutrunRace::signedLeadMeters() const {
    return signedLeadMeters_;
}

float OutrunRace::holdSeconds() const {
    return holdSeconds_;
}

float OutrunRace::holdProgress01() const {
    const float required =
        std::max(tuning_.leadHoldSeconds, 0.0f);

    if (required <= 0.0f) {
        return thresholdLeader_ == OutrunLeader::Tied
            ? 0.0f
            : 1.0f;
    }

    return std::clamp(holdSeconds_ / required, 0.0f, 1.0f);
}

const char* outrunOutcomeName(OutrunOutcome outcome) {
    switch (outcome) {
        case OutrunOutcome::InProgress: return "InProgress";
        case OutrunOutcome::PlayerWon: return "PlayerWon";
        case OutrunOutcome::RivalWon: return "RivalWon";
        case OutrunOutcome::Draw: return "Draw";
        case OutrunOutcome::Aborted: return "Aborted";
    }

    return "Unknown";
}

const char* outrunLeaderName(OutrunLeader leader) {
    switch (leader) {
        case OutrunLeader::Tied: return "Tied";
        case OutrunLeader::Player: return "Player";
        case OutrunLeader::Rival: return "Rival";
    }

    return "Unknown";
}

} // namespace frr::domain
