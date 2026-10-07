#include "VehicleSpatialProbe.h"

#include <windows.h>

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

enum class SlotReadKind : std::uint8_t {
    End,
    Ignored,
    Valid,
    Invalid,
    Fault
};

struct SlotReadResult {
    SlotReadKind kind = SlotReadKind::Fault;
    frr::domain::VehicleOrientedBox box{};
};

// Keep SEH in a POD-only helper. MSVC rejects __try in functions that need
// C++ object unwinding (VehicleSpatialSnapshot owns a std::vector).
SlotReadResult readVehicleSlot(
    std::uint32_t index
) {
    SlotReadResult out{};

#if defined(_MSC_VER)
    __try {
#endif
        using namespace NFSPluginSDK::MW05;

        const auto& slot =
            PVehicle::g_mInstances[index];

        PVehicle* raw = slot.mInstance;

        if (!raw) {
            out.kind = SlotReadKind::End;
            return out;
        }

        if (!slot.mIsEnabled) {
            out.kind = SlotReadKind::Ignored;
            return out;
        }

        auto* vehicle =
            raw | PVehicleEx::ValidatePVehicle;

        if (!vehicle) {
            out.kind = SlotReadKind::Invalid;
            out.box.identity =
                reinterpret_cast<std::uintptr_t>(raw);
            return out;
        }

        if (!vehicle->IsActive() ||
            vehicle->IsDestroyed()) {
            out.kind = SlotReadKind::Ignored;
            return out;
        }

        out.box.identity =
            reinterpret_cast<std::uintptr_t>(vehicle);

        IRigidBody* rigidBody =
            vehicle->GetRigidBody();

        if (!rigidBody ||
            vehicle->IsLoading()) {
            out.kind = SlotReadKind::Invalid;
            return out;
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
            out.kind = SlotReadKind::Invalid;
            return out;
        }

        out.box.center = copyVector(position);
        out.box.right = copyVector(right);
        out.box.up = copyVector(up);
        out.box.forward = copyVector(forward);
        out.box.halfExtents = copyVector(dimension);
        out.box.valid =
            frr::domain::validVehicleOrientedBox(
                out.box
            );

        out.kind = out.box.valid
            ? SlotReadKind::Valid
            : SlotReadKind::Invalid;

        return out;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out.kind = SlotReadKind::Fault;
        return out;
    }
#endif
}

} // namespace

VehicleSpatialSnapshot VehicleSpatialProbe::sample() {
    VehicleSpatialSnapshot out{};
    out.boxes.reserve(32);

    bool registryFault = false;

    for (std::uint32_t index = 0;
         index < kVehicleCountHardLimit;
         ++index) {
        const SlotReadResult slot =
            readVehicleSlot(index);

        if (slot.kind == SlotReadKind::End) {
            out.registryCount = index;
            out.registryComplete = !registryFault;
            break;
        }

        if (slot.kind == SlotReadKind::Ignored) {
            ++out.ignoredInactiveVehicles;
            continue;
        }

        ++out.enabledActiveVehicles;

        if (slot.kind == SlotReadKind::Valid) {
            out.boxes.push_back(slot.box);
            continue;
        }

        ++out.failedSpatialReads;
        out.boxes.push_back(slot.box);

        if (slot.kind == SlotReadKind::Fault) {
            // A fault means even the registry slot itself cannot be trusted.
            // Keep scanning bounded, but never call the fleet complete.
            registryFault = true;
            out.registryComplete = false;
        }
    }

    if (!out.registryComplete) {
        out.registryCount =
            kVehicleCountHardLimit;
    }

    return out;
}

} // namespace frr::game
