#pragma once

namespace frr::domain {

enum class StagingState {
    Search,
    Reserve,
    Approach,
    Align,
    CameraIntro,
    Negotiating,
    Ready,
    Countdown,
    Release,
    Completed,
    Aborted
};

struct StagingTuning {
    float searchTimeoutSeconds = 6.0f;
    float reserveTimeoutSeconds = 2.0f;
    float approachTimeoutSeconds = 10.0f;
    float alignmentTimeoutSeconds = 5.0f;
    float cameraIntroTimeoutSeconds = 10.0f;
    float negotiationTimeoutSeconds = 45.0f;
    float readyTimeoutSeconds = 5.0f;
    float countdownTimeoutSeconds = 8.0f;
    float releaseTimeoutSeconds = 3.0f;
    bool allowHiddenAlignmentFallback = true;
};

struct StagingInput {
    float deltaSeconds = 0.0f;

    bool playerValid = true;
    bool rivalValid = true;
    bool worldSafe = true;
    bool cancelRequested = false;

    bool stagingCandidateFound = false;
    bool reservationValid = false;
    bool approachComplete = false;
    bool vehiclesAligned = false;
    bool hiddenAlignmentTransformSafe = false;
    bool cameraIntroComplete = false;
    bool stakeConfirmed = false;
    bool enginesReady = false;
    bool countdownComplete = false;
    bool releaseComplete = false;
};

struct StagingUpdate {
    StagingState state = StagingState::Search;
    bool stateChanged = false;
    bool requestHiddenAlignment = false;
    bool suppressPlayerControl = false;
    bool cinematicCameraActive = false;
    bool restorePlayerControl = false;
    bool restoreCamera = false;
    bool completed = false;
    bool aborted = false;
};

class StagingStateMachine {
public:
    explicit StagingStateMachine(StagingTuning tuning = {});

    StagingUpdate tick(const StagingInput& input);
    StagingUpdate abort();
    void reset();

    StagingState state() const;
    float stateTimeSeconds() const;

private:
    void transition(StagingState next);
    StagingUpdate makeUpdate(
        StagingState before,
        bool hiddenAlignmentRequested = false
    ) const;

    StagingTuning tuning_{};
    StagingState state_ = StagingState::Search;
    float stateTimeSeconds_ = 0.0f;
};

const char* stagingStateName(StagingState state);

} // namespace frr::domain
