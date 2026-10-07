#pragma once

#include "VehicleSpatialEvidence.h"

#include <cstdint>

namespace frr::domain {

struct RenderMatrix4 {
    float m[4][4]{};
};

struct RenderFrustumSnapshot {
    bool captureValid = false;
    std::uint32_t viewportWidth = 0;
    std::uint32_t viewportHeight = 0;

    RenderMatrix4 view{};
    RenderMatrix4 projection{};
};

struct RenderCameraVerifierTuning {
    std::uint32_t requiredConsecutiveSamples = 4;

    // Intentionally loose. The player's rigid-body center is usually visible
    // in chase cameras, but camera modes can place it near/behind the eye.
    float playerNdcXYLimit = 3.0f;
    float playerNdcMinZ = -1.0f;
    float playerNdcMaxZ = 2.0f;
};

struct RenderCameraVerification {
    bool currentSnapshotPlausible = false;
    bool verified = false;

    std::uint32_t consecutivePlausibleSamples = 0;
    std::uint32_t acceptedSamples = 0;
    std::uint32_t rejectedSamples = 0;
};

struct FrustumVisibilityEvidence {
    bool visibilityVerified = false;

    // Conservative meaning:
    // false only when the complete candidate OBB is trivially rejected by
    // at least one homogeneous clip plane.
    bool visibleToPlayer = true;
    bool entirelyOutsideFrustum = false;

    std::uint32_t transformedCorners = 0;
};

bool finiteRenderMatrix(
    const RenderMatrix4& matrix
);

bool perspectiveProjectionLikely(
    const RenderMatrix4& projection
);

RenderMatrix4 multiplyRenderMatrices(
    const RenderMatrix4& left,
    const RenderMatrix4& right
);

bool playerPointPlausibleForRenderCamera(
    const RenderFrustumSnapshot& snapshot,
    const SpatialVector3& playerPoint,
    const RenderCameraVerifierTuning& tuning = {}
);

class RenderCameraVerifier {
public:
    explicit RenderCameraVerifier(
        RenderCameraVerifierTuning tuning = {}
    );

    void reset();

    RenderCameraVerification push(
        const RenderFrustumSnapshot& snapshot,
        const SpatialVector3& playerPoint
    );

    const RenderCameraVerification& state() const;

private:
    RenderCameraVerifierTuning tuning_{};
    RenderCameraVerification state_{};
};

FrustumVisibilityEvidence classifyVehicleAgainstFrustum(
    const RenderFrustumSnapshot& snapshot,
    const VehicleOrientedBox& candidate,
    bool cameraSemanticsVerified
);

} // namespace frr::domain
