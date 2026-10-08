#pragma once
#include "WorldCollisionEvidence.h"
#include <array>
#include <cmath>

namespace frr::domain {
inline std::array<SpatialVector3, 2> prototypeGroundSegment(SpatialVector3 point) {
    // CheckHitWorld faces the returned normal toward the ray's origin.
    // A local downward ray makes an upward normal meaningful and avoids
    // selecting distant decks across the old 1002-unit fallback segment.
    return {{{point.x, point.y + 2.0f, point.z}, {point.x, point.y - 4.0f, point.z}}};
}
inline bool prototypeGroundAcceptable(const GroundEvidence& ground) {
    return ground.groundVerified && ground.groundValid && ground.gradeVerified &&
        std::isfinite(ground.absoluteGrade) && ground.absoluteGrade <= 0.35f &&
        std::isfinite(ground.absoluteHeightDeltaWorldUnits) && ground.absoluteHeightDeltaWorldUnits <= 1.5f &&
        std::isfinite(ground.normal.y) && ground.normal.y >= 0.5f;
}
}
