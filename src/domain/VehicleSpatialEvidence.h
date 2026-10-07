#pragma once

#include <cstdint>
#include <vector>

namespace frr::domain {

struct SpatialVector3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct VehicleOrientedBox {
    bool valid = false;
    std::uintptr_t identity = 0;

    SpatialVector3 center{};
    SpatialVector3 right{};
    SpatialVector3 up{};
    SpatialVector3 forward{};

    // MW05 rigid-body dimensions are treated as local half-extents:
    // x = right/width, y = up/height, z = forward/length.
    SpatialVector3 halfExtents{};
};

struct PointOccupancyReport {
    bool queryValid = false;
    bool registryComplete = false;
    bool verified = false;

    bool insideAnyVehicle = false;
    std::uintptr_t containingVehicle = 0;

    unsigned checkedVehicles = 0;
    unsigned invalidVehicles = 0;

    float nearestSeparationWorldUnits = 0.0f;
    std::uintptr_t nearestVehicle = 0;
};

struct FleetOverlapReport {
    bool candidateValid = false;
    bool registryComplete = false;
    bool verified = false;
    bool overlaps = false;

    unsigned checkedVehicles = 0;
    unsigned invalidVehicles = 0;

    std::uintptr_t overlappingVehicle = 0;
};

bool validVehicleOrientedBox(
    const VehicleOrientedBox& box
);

bool pointInsideVehicleOrientedBox(
    const SpatialVector3& point,
    const VehicleOrientedBox& box
);

float pointSeparationFromVehicleOrientedBox(
    const SpatialVector3& point,
    const VehicleOrientedBox& box
);

bool vehicleOrientedBoxesOverlap(
    const VehicleOrientedBox& a,
    const VehicleOrientedBox& b,
    float paddingWorldUnits = 0.0f
);

PointOccupancyReport evaluatePointAgainstFleet(
    const SpatialVector3& point,
    const std::vector<VehicleOrientedBox>& fleet,
    bool registryComplete
);

FleetOverlapReport evaluateFootprintAgainstFleet(
    const VehicleOrientedBox& candidate,
    const std::vector<VehicleOrientedBox>& fleet,
    bool registryComplete,
    float paddingWorldUnits = 0.0f
);

} // namespace frr::domain
