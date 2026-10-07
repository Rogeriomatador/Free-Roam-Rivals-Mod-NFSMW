#include "SelectedRivalEvidence.h"

namespace frr::domain {
SelectedRivalOverlap evaluateSelectedRivalOverlap(
    SelectedRivalVehicle selected,
    const RoadCandidateObservation& candidate,
    const VehicleFootprintLearner& learner,
    const std::vector<VehicleOrientedBox>& fleet,
    bool registryComplete
) {
    SelectedRivalOverlap out{};
    if (selected.rivalId == 0 || selected.vehicleKey == 0 ||
        inspectRoadCandidate(candidate) != RoadCandidateBlocker::None) {
        return out;
    }
    const auto estimate = learner.estimate(selected.vehicleKey);
    if (!estimate.verified || estimate.vehicleKey != selected.vehicleKey) {
        return out;
    }
    // The largest observed dimensions cover every accepted sample; the mean
    // could underestimate a learned model by the permitted spread.
    out.footprint = makeRoadAlignedVehicleFootprint(
        selected.vehicleKey,
        {candidate.position.x, candidate.position.y, candidate.position.z},
        {candidate.forward.x, candidate.forward.y, candidate.forward.z},
        estimate.maximumHalfExtents
    );
    out.footprintVerified = validVehicleOrientedBox(out.footprint);
    if (!out.footprintVerified) return out;
    out.overlap = evaluateFootprintAgainstFleet(out.footprint, fleet, registryComplete);
    out.evidence.overlapVerified = out.overlap.verified;
    out.evidence.overlapsLiveVehicle = !out.overlap.verified || out.overlap.overlaps;
    return out;
}
} // namespace frr::domain
