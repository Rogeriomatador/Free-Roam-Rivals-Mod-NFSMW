#include "StagingStateMachine.h"

#include <algorithm>
#include <cmath>

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

    // Domain-only fail-closed rule. A verified in-game adapter must own BOTH
    // vehicles and prove reversible control before a street-duel countdown.
    // This does not pretend that mere INI flags or road geometry own inputs.
    if (tuning_.requireNativeStartAuthority &&
        (!std::isfinite(input.deltaSeconds) || input.deltaSeconds <= 0.0f ||
         input.deltaSeconds > 0.5f || !input.playerControlCustodyVerified ||
         !input.rivalBrakeCustodyVerified || !input.restorationVerified)) {
        transition(StagingState::Aborted);
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
            if (input.approachComplete) {
                transition(StagingState::Align);
            } else if (timedOut(stateTimeSeconds_, tuning_.approachTimeoutSeconds)) {
                // In a real duel, a timeout must NOT stand in for the rival
                // having actually reached the chosen road/formation.
                transition(tuning_.requireNativeStartAuthority
                    ? StagingState::Aborted : StagingState::Align);
            }
            break;

        case StagingState::Align:
            if (input.vehiclesAligned &&
                (!tuning_.requireNativeStartAuthority || input.bothStoppedVerified)) {
                transition(tuning_.cinematicIntroEnabled ? StagingState::CameraIntro :
                    tuning_.negotiationEnabled ? StagingState::Negotiating : StagingState::Ready);
            } else if (timedOut(stateTimeSeconds_, tuning_.alignmentTimeoutSeconds)) {
                if (!tuning_.requireNativeStartAuthority &&
                    tuning_.allowHiddenAlignmentFallback && input.hiddenAlignmentTransformSafe) {
                    hiddenAlignmentRequested = true;
                    transition(tuning_.cinematicIntroEnabled ? StagingState::CameraIntro :
                        tuning_.negotiationEnabled ? StagingState::Negotiating : StagingState::Ready);
                } else {
                    // Never teleport the player's or rival's live car to
                    // manufacture a visible, cutscene-free line-up.
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
            if (tuning_.requireNativeStartAuthority &&
                (!input.bothStoppedVerified || !input.vehiclesAligned)) {
                transition(StagingState::Aborted);
            } else if (input.enginesReady) {
                transition(StagingState::Countdown);
            } else if (timedOut(stateTimeSeconds_, tuning_.readyTimeoutSeconds)) {
                transition(StagingState::Aborted);
            }
            break;

        case StagingState::Countdown:
            if (tuning_.requireNativeStartAuthority &&
                (!input.bothStoppedVerified || !input.vehiclesAligned)) {
                // A moving car cannot receive a valid simultaneous start.
                transition(StagingState::Aborted);
            } else if (input.countdownComplete) {
                transition(StagingState::Release);
            } else if (timedOut(stateTimeSeconds_, tuning_.countdownTimeoutSeconds)) {
                transition(StagingState::Aborted);
            }
            break;

        case StagingState::Release:
            if (input.releaseComplete &&
                (!tuning_.requireNativeStartAuthority || input.nativeReleaseRestoredVerified)) {
                // The future adapter must ACK release/restoration on BOTH
                // vehicles; drawing GO! is not that ACK.
                transition(StagingState::Completed);
            } else if (timedOut(stateTimeSeconds_, tuning_.releaseTimeoutSeconds)) {
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
    out.cinematicCameraActive = tuning_.cinematicIntroEnabled && cameraActive(state_);
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
