#pragma once

#include "VehicleSpatialEvidence.h"

namespace frr::domain {

struct WorldCollisionSample {
    bool callAvailable = false;
    bool callCompleted = false;

    bool hit = false;
    unsigned char hitType = 0;

    SpatialVector3 hitPoint{};
    SpatialVector3 normal{};
    float engineDistance = 0.0f;
};

struct GroundEvidence {
    bool queryCompleted = false;
    bool groundVerified = false;
    bool groundValid = false;

    float groundY = 0.0f;
    float absoluteHeightDeltaWorldUnits = 0.0f;

    bool gradeVerified = false;
    float absoluteGrade = 0.0f;
    SpatialVector3 normal{};
};

struct WorldOcclusionEvidence {
    bool queryCompleted = false;
    bool occlusionVerified = false;
    bool occludedByWorld = false;

    SpatialVector3 firstHitPoint{};
    unsigned char hitType = 0;
};

GroundEvidence interpretGroundCollision(
    const SpatialVector3& candidate,
    const WorldCollisionSample& sample
);

WorldOcclusionEvidence interpretWorldOcclusion(
    const WorldCollisionSample& sample
);

} // namespace frr::domain
