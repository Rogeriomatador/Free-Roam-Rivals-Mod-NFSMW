#pragma once
#include "VehicleSpatialEvidence.h"
#include <cstdint>
#include <vector>

namespace frr::domain {
// Observations of active, spatially valid live IVehicle objects. NOT proof
// that the engine can construct another car of this model or that its
// texture/render-stream dependencies are resident at a new spawn location.
struct ActiveModelObservation {
    bool reliable=false;
    unsigned observed=0,otherThanPlayer=0;
};
inline ActiveModelObservation observeActiveModel(
    const std::vector<VehicleOrientedBox>& boxes,
    bool registryComplete,unsigned failedSpatialReads,
    std::uint32_t requestedKey,std::uintptr_t playerIVehicle) {
    ActiveModelObservation result{};
    result.reliable=registryComplete&&failedSpatialReads==0;
    if(!result.reliable||requestedKey==0||playerIVehicle==0) return result;
    for(const auto& box:boxes) {
        if(!box.valid||box.vehicleKey!=requestedKey||box.identity==0) continue;
        ++result.observed;
        if(box.identity!=playerIVehicle) ++result.otherThanPlayer;
    }
    return result;
}
} // namespace frr::domain
