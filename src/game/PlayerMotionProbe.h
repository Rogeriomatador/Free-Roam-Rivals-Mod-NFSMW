#pragma once

#include <cstdint>

namespace frr::game {

struct MotionVectorProbe {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    bool finite = false;
};

struct PlayerMotionProbe {
    bool available = false;

    std::uintptr_t pVehicle = 0;

    float speed = 0.0f;
    float speedometer = 0.0f;
    float absoluteSpeed = 0.0f;
    float slipAngle = 0.0f;

    std::uint32_t wheelsOnGround = 0;

    MotionVectorProbe position{};
    MotionVectorProbe localVelocity{};
    MotionVectorProbe linearVelocity{};

    float localVelocityMagnitude = 0.0f;
    float linearVelocityMagnitude = 0.0f;
};

class PlayerMotionProbeReader {
public:
    static PlayerMotionProbe sample(
        std::uintptr_t playerPVehicle
    );
};

} // namespace frr::game
