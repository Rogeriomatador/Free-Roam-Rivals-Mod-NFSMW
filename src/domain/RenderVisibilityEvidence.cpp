#include "RenderVisibilityEvidence.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace frr::domain {
namespace {

constexpr float kClipEpsilon = 1.0e-5f;

struct ClipVector4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

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

SpatialVector3 scale(
    const SpatialVector3& value,
    float amount
) {
    return {
        value.x * amount,
        value.y * amount,
        value.z * amount
    };
}

SpatialVector3 add(
    const SpatialVector3& a,
    const SpatialVector3& b
) {
    return {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
}

ClipVector4 transformPoint(
    const SpatialVector3& point,
    const RenderMatrix4& matrix
) {
    ClipVector4 out{};

    out.x =
        point.x * matrix.m[0][0] +
        point.y * matrix.m[1][0] +
        point.z * matrix.m[2][0] +
        matrix.m[3][0];

    out.y =
        point.x * matrix.m[0][1] +
        point.y * matrix.m[1][1] +
        point.z * matrix.m[2][1] +
        matrix.m[3][1];

    out.z =
        point.x * matrix.m[0][2] +
        point.y * matrix.m[1][2] +
        point.z * matrix.m[2][2] +
        matrix.m[3][2];

    out.w =
        point.x * matrix.m[0][3] +
        point.y * matrix.m[1][3] +
        point.z * matrix.m[2][3] +
        matrix.m[3][3];

    return out;
}

bool finiteClip(
    const ClipVector4& value
) {
    return
        finite(value.x) &&
        finite(value.y) &&
        finite(value.z) &&
        finite(value.w);
}

std::array<SpatialVector3, 8> boxCorners(
    const VehicleOrientedBox& box
) {
    std::array<SpatialVector3, 8> corners{};

    std::size_t index = 0;

    for (int sx : {-1, 1}) {
        for (int sy : {-1, 1}) {
            for (int sz : {-1, 1}) {
                SpatialVector3 point = box.center;
                point = add(
                    point,
                    scale(
                        box.right,
                        static_cast<float>(sx) *
                            box.halfExtents.x
                    )
                );
                point = add(
                    point,
                    scale(
                        box.up,
                        static_cast<float>(sy) *
                            box.halfExtents.y
                    )
                );
                point = add(
                    point,
                    scale(
                        box.forward,
                        static_cast<float>(sz) *
                            box.halfExtents.z
                    )
                );

                corners[index++] = point;
            }
        }
    }

    return corners;
}

} // namespace

bool finiteRenderMatrix(
    const RenderMatrix4& matrix
) {
    for (const auto& row : matrix.m) {
        for (float value : row) {
            if (!finite(value)) {
                return false;
            }
        }
    }

    return true;
}

bool perspectiveProjectionLikely(
    const RenderMatrix4& projection
) {
    if (!finiteRenderMatrix(projection)) {
        return false;
    }

    // D3D9 perspective matrices normally place +/-1 in _34 and 0 in _44.
    // Keep the test tolerant to handedness and custom near/far settings.
    return
        std::abs(projection.m[2][3]) > 0.25f &&
        std::abs(projection.m[3][3]) < 0.25f;
}

RenderMatrix4 multiplyRenderMatrices(
    const RenderMatrix4& left,
    const RenderMatrix4& right
) {
    RenderMatrix4 out{};

    for (int row = 0; row < 4; ++row) {
        for (int column = 0; column < 4; ++column) {
            float value = 0.0f;

            for (int k = 0; k < 4; ++k) {
                value +=
                    left.m[row][k] *
                    right.m[k][column];
            }

            out.m[row][column] = value;
        }
    }

    return out;
}

bool playerPointPlausibleForRenderCamera(
    const RenderFrustumSnapshot& snapshot,
    const SpatialVector3& playerPoint,
    const RenderCameraVerifierTuning& tuning
) {
    if (!snapshot.captureValid ||
        snapshot.viewportWidth == 0 ||
        snapshot.viewportHeight == 0 ||
        !finiteVector(playerPoint) ||
        !finiteRenderMatrix(snapshot.view) ||
        !perspectiveProjectionLikely(
            snapshot.projection
        ) ||
        !finite(tuning.playerNdcXYLimit) ||
        tuning.playerNdcXYLimit <= 0.0f ||
        !finite(tuning.playerNdcMinZ) ||
        !finite(tuning.playerNdcMaxZ) ||
        tuning.playerNdcMaxZ <
            tuning.playerNdcMinZ) {
        return false;
    }

    const RenderMatrix4 viewProjection =
        multiplyRenderMatrices(
            snapshot.view,
            snapshot.projection
        );

    const ClipVector4 clip =
        transformPoint(
            playerPoint,
            viewProjection
        );

    if (!finiteClip(clip) ||
        clip.w <= kClipEpsilon) {
        return false;
    }

    const float ndcX = clip.x / clip.w;
    const float ndcY = clip.y / clip.w;
    const float ndcZ = clip.z / clip.w;

    return
        finite(ndcX) &&
        finite(ndcY) &&
        finite(ndcZ) &&
        std::abs(ndcX) <=
            tuning.playerNdcXYLimit &&
        std::abs(ndcY) <=
            tuning.playerNdcXYLimit &&
        ndcZ >= tuning.playerNdcMinZ &&
        ndcZ <= tuning.playerNdcMaxZ;
}

RenderCameraVerifier::RenderCameraVerifier(
    RenderCameraVerifierTuning tuning
)
    : tuning_(tuning) {
    if (tuning_.requiredConsecutiveSamples == 0) {
        tuning_.requiredConsecutiveSamples = 1;
    }

    if (!finite(tuning_.playerNdcXYLimit) ||
        tuning_.playerNdcXYLimit <= 0.0f) {
        tuning_.playerNdcXYLimit = 3.0f;
    }

    if (!finite(tuning_.playerNdcMinZ) ||
        !finite(tuning_.playerNdcMaxZ) ||
        tuning_.playerNdcMaxZ <
            tuning_.playerNdcMinZ) {
        tuning_.playerNdcMinZ = -1.0f;
        tuning_.playerNdcMaxZ = 2.0f;
    }
}

void RenderCameraVerifier::reset() {
    state_ = {};
}

RenderCameraVerification
RenderCameraVerifier::push(
    const RenderFrustumSnapshot& snapshot,
    const SpatialVector3& playerPoint
) {
    const bool plausible =
        playerPointPlausibleForRenderCamera(
            snapshot,
            playerPoint,
            tuning_
        );

    state_.currentSnapshotPlausible =
        plausible;

    if (plausible) {
        ++state_.acceptedSamples;
        ++state_.consecutivePlausibleSamples;
    } else {
        ++state_.rejectedSamples;
        state_.consecutivePlausibleSamples = 0;
    }

    state_.verified =
        state_.consecutivePlausibleSamples >=
            tuning_.requiredConsecutiveSamples;

    return state_;
}

const RenderCameraVerification&
RenderCameraVerifier::state() const {
    return state_;
}

FrustumVisibilityEvidence
classifyVehicleAgainstFrustum(
    const RenderFrustumSnapshot& snapshot,
    const VehicleOrientedBox& candidate,
    bool cameraSemanticsVerified
) {
    FrustumVisibilityEvidence out{};

    if (!cameraSemanticsVerified ||
        !snapshot.captureValid ||
        snapshot.viewportWidth == 0 ||
        snapshot.viewportHeight == 0 ||
        !finiteRenderMatrix(snapshot.view) ||
        !perspectiveProjectionLikely(
            snapshot.projection
        ) ||
        !validVehicleOrientedBox(candidate)) {
        return out;
    }

    const RenderMatrix4 viewProjection =
        multiplyRenderMatrices(
            snapshot.view,
            snapshot.projection
        );

    bool allLeft = true;
    bool allRight = true;
    bool allBottom = true;
    bool allTop = true;
    bool allNear = true;
    bool allFar = true;

    const auto corners = boxCorners(candidate);

    for (const auto& corner : corners) {
        const ClipVector4 clip =
            transformPoint(
                corner,
                viewProjection
            );

        if (!finiteClip(clip)) {
            return {};
        }

        ++out.transformedCorners;

        allLeft &=
            clip.x < -clip.w;
        allRight &=
            clip.x > clip.w;
        allBottom &=
            clip.y < -clip.w;
        allTop &=
            clip.y > clip.w;
        allNear &=
            clip.z < 0.0f;
        allFar &=
            clip.z > clip.w;
    }

    out.visibilityVerified = true;

    out.entirelyOutsideFrustum =
        allLeft ||
        allRight ||
        allBottom ||
        allTop ||
        allNear ||
        allFar;

    // Fail-safe policy: if the OBB is not provably outside the frustum,
    // treat it as visible even if it is only intersecting/ambiguous.
    out.visibleToPlayer =
        !out.entirelyOutsideFrustum;

    return out;
}

} // namespace frr::domain
