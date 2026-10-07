#include "StagingPlanner.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace frr::domain {

StagingCandidateScore scoreStagingCandidate(
    const StagingCandidate& candidate,
    const StagingPlannerTuning& tuning
) {
    const float minAhead =
        std::max(tuning.minimumAheadMeters, 0.0f);
    const float maxAhead =
        std::max(tuning.maximumAheadMeters, minAhead);
    const float minWidth =
        std::max(tuning.minimumRoadWidthMeters, 0.0f);
    const float maxCurvature =
        std::max(tuning.maximumAbsoluteCurvature, 0.0f);
    const float maxGrade =
        std::max(tuning.maximumAbsoluteGrade, 0.0f);

    if (!candidate.roadValid ||
        !candidate.streamed ||
        candidate.junction ||
        candidate.obstructed ||
        !candidate.groundValid ||
        !candidate.supportsTwoCars ||
        candidate.distanceAheadMeters < minAhead ||
        candidate.distanceAheadMeters > maxAhead ||
        candidate.roadWidthMeters < minWidth ||
        candidate.absoluteCurvature > maxCurvature ||
        candidate.absoluteGrade > maxGrade) {
        return {};
    }

    const float preferred = std::clamp(
        tuning.preferredAheadMeters,
        minAhead,
        maxAhead
    );
    const float distanceSpan =
        std::max(maxAhead - minAhead, 1.0f);
    const float distancePenalty =
        std::abs(candidate.distanceAheadMeters - preferred) /
        distanceSpan;

    const float curvaturePenalty = maxCurvature > 0.0f
        ? candidate.absoluteCurvature / maxCurvature
        : 0.0f;
    const float gradePenalty = maxGrade > 0.0f
        ? candidate.absoluteGrade / maxGrade
        : 0.0f;
    const float widthBonus = minWidth > 0.0f
        ? std::min(candidate.roadWidthMeters / minWidth, 2.0f) - 1.0f
        : 0.0f;

    StagingCandidateScore out{};
    out.eligible = true;
    out.score =
        100.0f -
        distancePenalty * 25.0f -
        curvaturePenalty * 35.0f -
        gradePenalty * 20.0f +
        widthBonus * 15.0f;
    return out;
}

std::optional<std::size_t> selectBestStagingCandidate(
    const std::vector<StagingCandidate>& candidates,
    const StagingPlannerTuning& tuning
) {
    std::optional<std::size_t> bestIndex;
    float bestScore = -std::numeric_limits<float>::infinity();

    for (std::size_t i = 0; i < candidates.size(); ++i) {
        const auto scored = scoreStagingCandidate(candidates[i], tuning);
        if (!scored.eligible) {
            continue;
        }

        if (!bestIndex || scored.score > bestScore) {
            bestIndex = i;
            bestScore = scored.score;
        }
    }

    return bestIndex;
}

} // namespace frr::domain
