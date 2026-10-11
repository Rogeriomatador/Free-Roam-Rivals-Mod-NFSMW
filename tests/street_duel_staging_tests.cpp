#include "domain/StagingStateMachine.h"
#include <cstdlib>
#include <iostream>
#include <limits>

using namespace frr::domain;

static void require(bool ok, const char* why) {
    if (!ok) { std::cerr << "FAILED: " << why << '\n'; std::exit(1); }
}
static StagingInput verified() {
    StagingInput in{};
    in.deltaSeconds = 0.016f;
    in.playerControlCustodyVerified = true;
    in.rivalBrakeCustodyVerified = true;
    in.restorationVerified = true;
    return in;
}
static void toAlignment(StagingStateMachine& duel, StagingInput& in) {
    in.stagingCandidateFound = true;
    duel.tick(in);
    require(duel.state() == StagingState::Reserve, "safe candidate");
    in.reservationValid = true;
    duel.tick(in);
    require(duel.state() == StagingState::Approach, "reservation");
    in.approachComplete = true;
    duel.tick(in);
    require(duel.state() == StagingState::Align, "physical approach");
}
static void toCountdown(StagingStateMachine& duel, StagingInput& in) {
    toAlignment(duel, in);
    in.vehiclesAligned = true;
    in.bothStoppedVerified = true;
    auto result = duel.tick(in);
    require(duel.state() == StagingState::Ready, "no cinema or negotiation");
    require(!result.cinematicCameraActive && !result.requestHiddenAlignment,
        "no cutscene or invisible car teleport");
    in.enginesReady = true;
    result = duel.tick(in);
    require(duel.state() == StagingState::Countdown && result.suppressPlayerControl,
        "real start is not a rolling Outrun");
    require(!result.cinematicCameraActive, "no camera takeover on countdown");
}
int main() {
    auto tuning = streetDuelStagingTuning();
    require(!tuning.cinematicIntroEnabled && !tuning.allowHiddenAlignmentFallback &&
        !tuning.negotiationEnabled && tuning.requireNativeStartAuthority,
        "street duel strict contract");

    { // A request must not manufacture player custody or rival braking.
        StagingStateMachine duel(tuning);
        StagingInput in{};
        in.stagingCandidateFound = true;
        auto result = duel.tick(in);
        require(result.aborted && result.restorePlayerControl &&
            !result.suppressPlayerControl && !result.cinematicCameraActive,
            "no native control ownership, no staging mutation");
    }
    {
        StagingStateMachine duel(tuning);
        auto in = verified();
        in.rivalBrakeCustodyVerified = false;
        require(duel.tick(in).aborted, "rival native brake ABI required");
    }
    {
        StagingStateMachine duel(tuning);
        auto in = verified();
        in.restorationVerified = false;
        require(duel.tick(in).aborted, "restoration must be proven first");
    }
    {
        StagingStateMachine duel(tuning);
        auto in = verified();
        in.deltaSeconds = std::numeric_limits<float>::quiet_NaN();
        require(duel.tick(in).aborted, "invalid physics time cannot authorize a start");
    }
    {
        StagingStateMachine duel(tuning);
        auto in = verified();
        in.stagingCandidateFound = true;
        duel.tick(in);
        in.reservationValid = true;
        duel.tick(in);
        in.deltaSeconds = tuning.approachTimeoutSeconds + 0.1f;
        require(duel.tick(in).aborted, "invalid long frame must fail closed");
    }
    {
        StagingStateMachine duel(tuning);
        auto in = verified();
        toAlignment(duel, in);
        in.hiddenAlignmentTransformSafe = true; // must not trigger teleport
        in.deltaSeconds = 0.5f;
        for (int i = 0; i < 11; ++i) {
            auto result = duel.tick(in);
            require(!result.requestHiddenAlignment, "strict duel cannot teleport");
        }
        require(duel.state() == StagingState::Aborted, "alignment timeout cancels duel");
    }
    {
        StagingStateMachine duel(tuning);
        auto in = verified();
        toCountdown(duel, in);
        in.countdownComplete = true;
        in.bothStoppedVerified = false;
        auto result = duel.tick(in);
        require(result.aborted && result.restorePlayerControl, "moving during countdown aborts");
    }
    {
        StagingStateMachine duel(tuning);
        auto in = verified();
        toCountdown(duel, in);
        in.rivalBrakeCustodyVerified = false;
        auto result = duel.tick(in);
        require(result.aborted && result.restorePlayerControl, "lost AI control aborts");
    }
    {
        StagingStateMachine duel(tuning);
        auto in = verified();
        toCountdown(duel, in);
        in.countdownComplete = true;
        auto release = duel.tick(in);
        require(release.state == StagingState::Release && !release.completed,
            "three-two-one alone does not begin the race");
        in.releaseComplete = true;
        release = duel.tick(in);
        require(!release.completed && release.state == StagingState::Release,
            "release ACK requires verified native restoration");
        in.nativeReleaseRestoredVerified = true;
        release = duel.tick(in);
        require(release.completed && release.restorePlayerControl &&
            !release.suppressPlayerControl && !release.cinematicCameraActive,
            "start completes only when release and restore are both verified");
    }
    {
        // Existing cinematics-based simulation remains unchanged outside
        // the opt-in strict street-duel preset.
        StagingStateMachine legacy;
        StagingInput in{};
        toAlignment(legacy, in);
        in.vehiclesAligned = true;
        auto result = legacy.tick(in);
        require(result.state == StagingState::CameraIntro &&
            result.cinematicCameraActive, "legacy staging remains compatible");
    }
    std::cout << "Street duel: no-cutscene, no-teleport, custody and release gates passed\n";
}
