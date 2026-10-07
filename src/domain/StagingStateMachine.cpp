#include "StagingStateMachine.h"

#include <algorithm>

namespace frr::domain {
namespace {

bool timedOut(float stateTime, float limit) {
    return limit > 0.0f && stateTime >= limit;
}

bool controlSuppressed(StagingState state) {
    switch (state) {
        case StagingState::Align:
        case StagingState::CameraIntro:
        case StagingState::Negotiating:
        case StagingState::Ready:
        case StagingState::Countdown:
        case StagingState::Release:
            return true;
        default:
            return false;
    }
}

bool cameraActive(StagingState state) {
    switch (state) {
        case StagingState::CameraIntro:
        case StagingState::Negotiating:
        case StagingState::Ready:
        case StagingState::Countdown:
        case StagingState::Release:
            return true;
        default:
            return false;
    }
}

} // namespace

StagingStateMachine::StagingStateMachine(
    StagingTuning tuning
) : tuning_(tuning) {}

void StagingStateMachine::transition(StagingState next) {
    state_ = next;
    stateTimeSeconds_ = 0.0f;
}

StagingUpdate StagingStateMachine::tick(
    const StagingInput& input
) {
    const StagingState before = state_;

    if (state_ == StagingState::Completed ||
        state_ == StagingState::Aborted) {
        return makeUpdate(before);
    }

    stateTimeSeconds_ += std::max(input.deltaSeconds, 0.0f);

    if (!input.playerValid ||
        !input.rivalValid ||
        !input.worldSafe ||
        input.cancelRequested) {
        transition(StagingState::Aborted);
        return makeUpdate(before);
    }

    bool hiddenAlignmentRequested = false;

    switch (state_) {
        case StagingState::Search:
            if (input.stagingCandidateFound) {
                transition(StagingState::Reserve);
            } else if (timedOut(
                           stateTimeSeconds_,
                           tuning_.searchTimeoutSeconds)) {
                transition(StagingState::Aborted);
            }
            break;

        case StagingState::Reserve:
            if (input.reservationValid) {
                transition(StagingState::Approach);
            } else if (timedOut(
                           stateTimeSeconds_,
                           tuning_.reserveTimeoutSeconds)) {
                transition(StagingState::Search);
            }
            break;

        case StagingState::Approach:
            if (input.approachComplete ||
                timedOut(
                    stateTimeSeconds_,
                    tuning_.approachTimeoutSeconds)) {
                transition(StagingState::Align);
            }
            break;

        case StagingState::Align:
            if (input.vehiclesAligned) {
                transition(StagingState::CameraIntro);
            } else if (timedOut(
                           stateTimeSeconds_,
                           tuning_.alignmentTimeoutSeconds)) {
                if (tuning_.allowHiddenAlignmentFallback &&
                    input.hiddenAlignmentTransformSafe) {
                    hiddenAlignmentRequested = true;
                    transition(StagingState::CameraIntro);
                } else {
                    transition(StagingState::Aborted);
                }
            }
            break;

        case StagingState::CameraIntro:
            if (input.cameraIntroComplete) {
                transition(StagingState::Negotiating);
            } else if (timedOut(
                           stateTimeSeconds_,
                           tuning_.cameraIntroTimeoutSeconds)) {
                transition(StagingState::Aborted);
            }
            break;

        case StagingState::Negotiating:
            if (input.stakeConfirmed) {
                transition(StagingState::Ready);
            } else if (timedOut(
                           stateTimeSeconds_,
                           tuning_.negotiationTimeoutSeconds)) {
                transition(StagingState::Aborted);
            }
            break;

        case StagingState::Ready:
            if (input.enginesReady) {
                transition(StagingState::Countdown);
            } else if (timedOut(
                           stateTimeSeconds_,
                           tuning_.readyTimeoutSeconds)) {
                transition(StagingState::Aborted);
            }
            break;

        case StagingState::Countdown:
            if (input.countdownComplete) {
                transition(StagingState::Release);
            } else if (timedOut(
                           stateTimeSeconds_,
                           tuning_.countdownTimeoutSeconds)) {
                transition(StagingState::Aborted);
            }
            break;

        case StagingState::Release:
            if (input.releaseComplete) {
                transition(StagingState::Completed);
            } else if (timedOut(
                           stateTimeSeconds_,
                           tuning_.releaseTimeoutSeconds)) {
                transition(StagingState::Aborted);
            }
            break;

        case StagingState::Completed:
        case StagingState::Aborted:
            break;
    }

    return makeUpdate(before, hiddenAlignmentRequested);
}

StagingUpdate StagingStateMachine::abort() {
    const StagingState before = state_;
    if (state_ != StagingState::Completed) {
        transition(StagingState::Aborted);
    }
    return makeUpdate(before);
}

void StagingStateMachine::reset() {
    state_ = StagingState::Search;
    stateTimeSeconds_ = 0.0f;
}

StagingState StagingStateMachine::state() const {
    return state_;
}

float StagingStateMachine::stateTimeSeconds() const {
    return stateTimeSeconds_;
}

StagingUpdate StagingStateMachine::makeUpdate(
    StagingState before,
    bool hiddenAlignmentRequested
) const {
    StagingUpdate out{};
    out.state = state_;
    out.stateChanged = state_ != before;
    out.requestHiddenAlignment = hiddenAlignmentRequested;
    out.suppressPlayerControl = controlSuppressed(state_);
    out.cinematicCameraActive = cameraActive(state_);
    out.completed = state_ == StagingState::Completed;
    out.aborted = state_ == StagingState::Aborted;

    if (out.completed || out.aborted) {
        out.restorePlayerControl = true;
        out.restoreCamera = true;
        out.suppressPlayerControl = false;
        out.cinematicCameraActive = false;
    }

    return out;
}

const char* stagingStateName(StagingState state) {
    switch (state) {
        case StagingState::Search: return "Search";
        case StagingState::Reserve: return "Reserve";
        case StagingState::Approach: return "Approach";
        case StagingState::Align: return "Align";
        case StagingState::CameraIntro: return "CameraIntro";
        case StagingState::Negotiating: return "Negotiating";
        case StagingState::Ready: return "Ready";
        case StagingState::Countdown: return "Countdown";
        case StagingState::Release: return "Release";
        case StagingState::Completed: return "Completed";
        case StagingState::Aborted: return "Aborted";
    }

    return "Unknown";
}

} // namespace frr::domain
