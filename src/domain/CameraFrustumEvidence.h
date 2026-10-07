#pragma once
#include "VehicleSpatialEvidence.h"

namespace frr::domain {
struct CameraMatrix4 { float m[4][4]{}; };
struct PrimaryCameraSample {
    bool available = false;
    bool active = false;
    bool coordinateMappingEstablished = false;
    unsigned viewId = 0;
    SpatialVector3 eyeRender{};
    CameraMatrix4 view{};
    CameraMatrix4 projection{};
    CameraMatrix4 viewProjection{};
};
enum class CameraBoxVisibility { Unverified, OutsideFrustum, PotentiallyVisible };
struct CameraFrustumReport {
    bool queryVerified = false;
    CameraBoxVisibility visibility = CameraBoxVisibility::Unverified;
    int separatingPlane = -1;
    // Main view/collision footprint alone never establishes final spawn visibility.
    bool spawnVisibilityVerified = false;
};
SpatialVector3 simulationToRender(const SpatialVector3& point);
bool coherentPrimaryCamera(const PrimaryCameraSample& sample);
CameraFrustumReport classifyPrimaryCameraFootprint(
    const PrimaryCameraSample& camera,
    const VehicleOrientedBox& simulationBox
);
const char* cameraBoxVisibilityName(CameraBoxVisibility visibility);
} // namespace frr::domain
