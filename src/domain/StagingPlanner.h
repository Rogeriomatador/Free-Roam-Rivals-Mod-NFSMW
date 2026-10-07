#pragma once

#include <cstddef>
#include <optional>
#include <vector>

namespace frr::domain {

struct StagingPlannerTuning {
    float minimumAheadMeters = 35.0f;
    float maximumAheadMeters = 220.0f;
    float preferredAheadMeters = 120.0f;
    float minimumRoadWidthMeters = 7.0f;
    float maximumAbsoluteCurvature = 0.025f;
    float maximumAbsoluteGrade = 0.18f;
};

struct StagingCandidate {
    // This must be true only after engine/world units have been explicitly
    // calibrated to metres. Raw WRoadNav coordinate deltas are not enough.
    bool metricGeometryVerified = false;

    float distanceAheadMeters = 0.0f;
    float roadWidthMeters = 0.0f;
    float absoluteCurvature = 0.0f;
    float absoluteGrade = 0.0f;

    bool roadValid = false;
    bool streamed = false;
    bool junction = false;
    bool obstructed = false;
    bool groundValid = false;
    bool supportsTwoCars = false;
};

struct StagingCandidateScore {
    bool eligible = false;
    float score = 0.0f;
};

StagingCandidateScore scoreStagingCandidate(
    const StagingCandidate& candidate,
    const StagingPlannerTuning& tuning = {}
);

std::optional<std::size_t> selectBestStagingCandidate(
    const std::vector<StagingCandidate>& candidates,
    const StagingPlannerTuning& tuning = {}
);

} // namespace frr::domain
