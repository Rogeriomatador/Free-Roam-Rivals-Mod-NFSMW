#include "WorldCollisionEvidence.h"

#include <cmath>

namespace frr::domain {
namespace {

constexpr float kNormalEpsilon = 1.0e-4f;

bool finite(float value) {
    return std::isfinite(value);
}

bool finiteVector(
    const SpatialVector3& value
) {
    return
        finite(value.x) &&
        finite(value.y) &&
        finite(value.z);
}

float length(
    const SpatialVector3& value
) {
    const float squared =
        value.x * value.x +
        value.y * value.y +
        value.z * value.z;

    if (!finite(squared) ||
        squared <= 0.0f) {
        return 0.0f;
    }

    const float result = std::sqrt(squared);
    return finite(result) ? result : 0.0f;
}

} // namespace

GroundEvidence interpretGroundCollision(
    const SpatialVector3& candidate,
    const WorldCollisionSample& sample
) {
    GroundEvidence out{};
    out.queryCompleted = sample.callCompleted;

    if (!sample.callAvailable ||
        !sample.callCompleted ||
        !sample.hit ||
        sample.hitType != 1 ||
        !finiteVector(candidate) ||
        !finiteVector(sample.hitPoint) ||
        !finiteVector(sample.normal)) {
        return out;
    }

    const float normalLength =
        length(sample.normal);

    if (normalLength <= kNormalEpsilon) {
        return out;
    }

    const float nx =
        sample.normal.x / normalLength;
    const float ny =
        sample.normal.y / normalLength;
    const float nz =
        sample.normal.z / normalLength;

    if (!finite(nx) ||
        !finite(ny) ||
        !finite(nz) ||
        std::abs(ny) <= kNormalEpsilon) {
        return out;
    }

    out.groundVerified = true;
    out.groundValid = true;
    out.groundY = sample.hitPoint.y;
    out.absoluteHeightDeltaWorldUnits =
        std::abs(candidate.y - sample.hitPoint.y);

    out.normal = {
        nx,
        ny,
        nz
    };

    const float horizontal =
        std::sqrt(nx * nx + nz * nz);

    if (finite(horizontal)) {
        out.gradeVerified = true;
        out.absoluteGrade =
            horizontal / std::abs(ny);
    }

    return out;
}

WorldOcclusionEvidence interpretWorldOcclusion(
    const WorldCollisionSample& sample
) {
    WorldOcclusionEvidence out{};
    out.queryCompleted = sample.callCompleted;

    if (!sample.callAvailable ||
        !sample.callCompleted) {
        return out;
    }

    out.occlusionVerified = true;
    out.occludedByWorld = sample.hit;
    out.firstHitPoint = sample.hitPoint;
    out.hitType = sample.hitType;
    return out;
}

} // namespace frr::domain
