#pragma once
#include "../domain/CameraFrustumEvidence.h"
namespace frr::game {
class CameraFrustumProbe {
public:
    // Guarded data reads only, sampled with road/vehicle evidence at EndScene.
    static frr::domain::PrimaryCameraSample sample();
};
} // namespace frr::game
