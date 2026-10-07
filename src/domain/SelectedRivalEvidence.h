#pragma once
#include "RoadCandidatePlanner.h"
#include "VehicleFootprintLearning.h"

namespace frr::domain {
struct SelectedRivalVehicle {
    std::uint64_t rivalId = 0;
    std::uint32_t vehicleKey = 0;
};
struct SelectedRivalOverlap {
    bool footprintVerified = false;
    VehicleOrientedBox footprint{};
    FleetOverlapReport overlap{};
    RoadCandidateEvidence evidence{};
};
// Never substitutes another learned model for the selected rival's vehicle.
SelectedRivalOverlap evaluateSelectedRivalOverlap(
    SelectedRivalVehicle selected,
    const RoadCandidateObservation& candidate,
    const VehicleFootprintLearner& learner,
    const std::vector<VehicleOrientedBox>& fleet,
    bool registryComplete
);
} // namespace frr::domain
