#pragma once

#include "../domain/VehicleSpatialEvidence.h"
#include "../domain/WorldCollisionEvidence.h"

namespace frr::game {

class WorldCollisionProbe {
public:
    // Verified for the project's exact supported RELOADED 1.3 executable.
    static constexpr unsigned long kCheckHitWorldAddress =
        0x007854B0ul;

    static bool addressAvailable();

    // Vertical world-face probe around a candidate. Uses primitive mask 1
    // (world faces) and does not mutate collision state.
    static frr::domain::WorldCollisionSample
    sampleGround(
        const frr::domain::SpatialVector3& point
    );
    // Local downward face query for the prototype's upward-normal gate.
    static frr::domain::WorldCollisionSample samplePrototypeGround(
        const frr::domain::SpatialVector3& point);

    // World/barrier occlusion probe between two points. This is NOT camera
    // frustum visibility and must not directly set visibilityVerified.
    static frr::domain::WorldCollisionSample
    sampleWorldOcclusion(
        const frr::domain::SpatialVector3& from,
        const frr::domain::SpatialVector3& to
    );
};

} // namespace frr::game
