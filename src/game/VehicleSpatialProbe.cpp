#include "VehicleSpatialProbe.h"

#include <NFSPluginSDK/Game.MW05/MW05.h>
#include <NFSPluginSDK/Game.MW05/Extensions.h>

#include <cmath>
#include <cstdint>

namespace frr::game {
namespace {

constexpr std::uint32_t kVehicleCountHardLimit = 512u;

frr::domain::SpatialVector3 copyVector(
    const NFSPluginSDK::MW05::UMath::Vector3& value
) {
    return {
        value.x,
        value.y,
        value.z
    };
}

bool finiteVector(
    const NFSPluginSDK::MW05::UMath::Vector3& value
) {
    return
        std::isfinite(value.x) &&
        std::isfinite(value.y) &&
        std::isfinite(value.z);
}

} // namespace

VehicleSpatialSnapshot VehicleSpatialProbe::sample() {
    VehicleSpatialSnapshot out{};

#if defined(_MSC_VER)
    __try {
#endif
        using namespace NFSPluginSDK::MW05;

        out.boxes.reserve(32);

        std::uint32_t count = 0;

        for (; count < kVehicleCountHardLimit; ++count) {
            const auto& slot =
                PVehicle::g_mInstances[count];

            PVehicle* raw = slot.mInstance;
            if (!raw) {
                out.registryComplete = true;
                break;
            }

            auto* vehicle =
                raw | PVehicleEx::ValidatePVehicle;

            if (!vehicle) {
                ++out.failedSpatialReads;

                frr::domain::VehicleOrientedBox invalid{};
                invalid.identity =
                    reinterpret_cast<std::uintptr_t>(raw);
                out.boxes.push_back(invalid);
                continue;
            }

            if (!slot.mIsEnabled ||
                !vehicle->IsActive() ||
                vehicle->IsDestroyed()) {
                ++out.ignoredInactiveVehicles;
                continue;
            }

            ++out.enabledActiveVehicles;

            frr::domain::VehicleOrientedBox box{};
            box.identity =
                reinterpret_cast<std::uintptr_t>(vehicle);

            IRigidBody* rigidBody =
                vehicle->GetRigidBody();

            if (!rigidBody ||
                vehicle->IsLoading()) {
                ++out.failedSpatialReads;
                out.boxes.push_back(box);
                continue;
            }

            const UMath::Vector3& position =
                rigidBody->GetPosition();

            UMath::Vector3 right{};
            UMath::Vector3 up{};
            UMath::Vector3 forward{};
            UMath::Vector3 dimension{};

            rigidBody->GetRightVector(right);
            rigidBody->GetUpVector(up);
            rigidBody->GetForwardVector(forward);
            rigidBody->GetDimension(dimension);

            if (!finiteVector(position) ||
                !finiteVector(right) ||
                !finiteVector(up) ||
                !finiteVector(forward) ||
                !finiteVector(dimension) ||
                dimension.x <= 0.0f ||
                dimension.y <= 0.0f ||
                dimension.z <= 0.0f) {
                ++out.failedSpatialReads;
                out.boxes.push_back(box);
                continue;
            }

            box.center = copyVector(position);
            box.right = copyVector(right);
            box.up = copyVector(up);
            box.forward = copyVector(forward);
            box.halfExtents = copyVector(dimension);
            box.valid =
                frr::domain::validVehicleOrientedBox(
                    box
                );

            if (!box.valid) {
                ++out.failedSpatialReads;
            }

            out.boxes.push_back(box);
        }

        out.registryCount = count;

        if (count >= kVehicleCountHardLimit) {
            out.registryComplete = false;
        }
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out = VehicleSpatialSnapshot{};
    }
#endif

    return out;
}

} // namespace frr::game
