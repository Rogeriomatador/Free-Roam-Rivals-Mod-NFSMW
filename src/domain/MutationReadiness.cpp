#include "MutationReadiness.h"

namespace frr::domain {

MutationReadinessReport evaluateMutationReadiness(
    const MutationReadinessInput& input
) {
    MutationReadinessReport out{};

    if (!input.frameTickProbeEnabled) {
        out.blocker =
            MutationReadinessBlocker::FrameTickProbeDisabled;
        return out;
    }

    if (!input.frameTickProbeInstalled) {
        out.blocker =
            MutationReadinessBlocker::FrameTickProbeNotInstalled;
        return out;
    }

    if (input.frameTickCount == 0 ||
        input.frameTickThreadId == 0) {
        out.blocker =
            MutationReadinessBlocker::FrameTickNotObserved;
        return out;
    }

    if (input.inputPollCount == 0 ||
        input.inputThreadId == 0) {
        out.blocker =
            MutationReadinessBlocker::InputPollNotObserved;
        return out;
    }

    if (input.frameTickThreadId !=
        input.inputThreadId) {
        out.blocker =
            MutationReadinessBlocker::MainLoopThreadUnconfirmed;
        return out;
    }

    out.gameplayThreadConfirmed = true;

    if (!input.safeFreeRoamObserved) {
        out.blocker =
            MutationReadinessBlocker::FreeRoamNotObserved;
        return out;
    }

    if (!input.roadLookaheadObserved) {
        out.blocker =
            MutationReadinessBlocker::RoadLookaheadUnavailable;
        return out;
    }

    if (!input.exactRoadCandidateObserved) {
        out.blocker =
            MutationReadinessBlocker::ExactRoadCandidateUnavailable;
        return out;
    }

    if (!input.metricCalibrationVerified) {
        out.blocker =
            MutationReadinessBlocker::MetricCalibrationUnverified;
        return out;
    }

    if (!input.spawnCandidateVerified) {
        out.blocker =
            MutationReadinessBlocker::SpawnCandidateUnverified;
        return out;
    }

    out.blocker = MutationReadinessBlocker::None;
    out.readyForConstructionExperiment = true;
    return out;
}

const char* mutationReadinessBlockerName(
    MutationReadinessBlocker blocker
) {
    switch (blocker) {
        case MutationReadinessBlocker::None:
            return "None";
        case MutationReadinessBlocker::FrameTickProbeDisabled:
            return "FrameTickProbeDisabled";
        case MutationReadinessBlocker::FrameTickProbeNotInstalled:
            return "FrameTickProbeNotInstalled";
        case MutationReadinessBlocker::FrameTickNotObserved:
            return "FrameTickNotObserved";
        case MutationReadinessBlocker::InputPollNotObserved:
            return "InputPollNotObserved";
        case MutationReadinessBlocker::MainLoopThreadUnconfirmed:
            return "MainLoopThreadUnconfirmed";
        case MutationReadinessBlocker::FreeRoamNotObserved:
            return "FreeRoamNotObserved";
        case MutationReadinessBlocker::RoadLookaheadUnavailable:
            return "RoadLookaheadUnavailable";
        case MutationReadinessBlocker::ExactRoadCandidateUnavailable:
            return "ExactRoadCandidateUnavailable";
        case MutationReadinessBlocker::MetricCalibrationUnverified:
            return "MetricCalibrationUnverified";
        case MutationReadinessBlocker::SpawnCandidateUnverified:
            return "SpawnCandidateUnverified";
    }

    return "Unknown";
}

} // namespace frr::domain
