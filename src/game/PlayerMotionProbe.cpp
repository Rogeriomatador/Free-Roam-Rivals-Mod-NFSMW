#include "PlayerMotionProbe.h"
#include "NfsPluginCoordinateAdapter.h"

#include <windows.h>

#include <NFSPluginSDK/Game.MW05/MW05.h>
#include <NFSPluginSDK/Game.MW05/Extensions.h>

#include <cmath>
#include <cstdint>

namespace frr::game {
namespace {

template <typename V>
MotionVectorProbe copyVector(const V& value) {
    const auto canonical = canonicalMwVector(value);
    MotionVectorProbe out{};
    out.x = canonical.x;
    out.y = canonical.y;
    out.z = canonical.z;
    out.finite =
        std::isfinite(out.x) &&
        std::isfinite(out.y) &&
        std::isfinite(out.z);
    return out;
}

float magnitude(
    const MotionVectorProbe& value
) {
    if (!value.finite) {
        return 0.0f;
    }

    const float result = std::sqrt(
        value.x * value.x +
        value.y * value.y +
        value.z * value.z
    );

    return std::isfinite(result)
        ? result
        : 0.0f;
}

} // namespace

PlayerMotionProbe PlayerMotionProbeReader::sample(
    std::uintptr_t playerPVehicle
) {
    PlayerMotionProbe out{};

    if (playerPVehicle == 0) {
        return out;
    }

#if defined(_MSC_VER)
    __try {
#endif
        using namespace NFSPluginSDK::MW05;

        auto* raw = reinterpret_cast<PVehicle*>(
            playerPVehicle
        );

        auto* player =
            raw | PVehicleEx::ValidatePVehicle;

        if (!player) {
            return out;
        }

        out.pVehicle = playerPVehicle;
        out.vehicleKey = player->GetVehicleKey();

        out.speed = player->GetSpeed();
        out.speedometer = player->GetSpeedometer();
        out.absoluteSpeed = player->GetAbsoluteSpeed();
        out.slipAngle = player->GetSlipAngle();
        out.wheelsOnGround = player->mWheelsOnGround;

        out.position =
            copyVector(player->GetPosition());

        out.localVelocity =
            copyVector(player->GetLocalVelocity());

        UMath::Vector3 linear{};
        player->GetLinearVelocity(linear);
        out.linearVelocity = copyVector(linear);

        out.localVelocityMagnitude =
            magnitude(out.localVelocity);

        out.linearVelocityMagnitude =
            magnitude(out.linearVelocity);

        out.available =
            std::isfinite(out.speed) &&
            std::isfinite(out.speedometer) &&
            std::isfinite(out.absoluteSpeed) &&
            std::isfinite(out.slipAngle) &&
            out.position.finite &&
            out.localVelocity.finite &&
            out.linearVelocity.finite;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out = PlayerMotionProbe{};
    }
#endif

    return out;
}

} // namespace frr::game
