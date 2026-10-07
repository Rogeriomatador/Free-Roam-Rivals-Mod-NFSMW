#include "VehicleSpatialProbe.h"

#include <windows.h>

#include <mwsdk/game/mw05.hpp>

#include <NFSPluginSDK/Game.MW05/MW05.h>

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
    Ignored,
    Valid,
    Invalid,
    Fault
};

struct SlotReadResult {
    SlotReadKind kind = SlotReadKind::Fault;
    frr::domain::VehicleOrientedBox box{};
};

// D40 in the pinned MWSDK proves that the live list contains IVehicle*
// interface subobjects. Do not subtract offsets or walk the stale PVehicle
// instance pool for occupancy. Call only the public IVehicle/ISimable virtual
// contract from each verified live-list entry and keep every failure fail-closed.
SlotReadResult readLiveVehicle(void* liveInterface) {
    SlotReadResult out{};

#if defined(_MSC_VER)
    __try {
#endif
        using namespace NFSPluginSDK::MW05;

        if (!liveInterface) {
            out.kind = SlotReadKind::Fault;
            return out;
        }

        auto* vehicle =
            reinterpret_cast<IVehicle*>(liveInterface);

        if (!vehicle->IsActive() ||
            vehicle->IsDestroyed()) {
            out.kind = SlotReadKind::Ignored;
            return out;
        }

        out.box.identity =
            reinterpret_cast<std::uintptr_t>(liveInterface);

        if (vehicle->IsLoading()) {
            out.kind = SlotReadKind::Invalid;
            return out;
        }

        out.box.vehicleKey =
            vehicle->GetVehicleKey();

        ISimable* simable =
            vehicle->GetSimable();

        if (!simable) {
            out.kind = SlotReadKind::Invalid;
            return out;
        }

        IRigidBody* rigidBody =
            simable->GetRigidBody();

        if (!rigidBody) {
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

        out.box =
            frr::domain::makeVehicleOrientedBox(
                out.box.identity,
                out.box.vehicleKey,
                copyVector(position),
                copyVector(right),
                copyVector(up),
                copyVector(forward),
                copyVector(dimension)
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

    const std::uint32_t count =
        mwsdk::mw05::vehicle_count();

    if (count > kVehicleCountHardLimit) {
        out.registryCount = count;
        out.registryComplete = false;
        return out;
    }

    out.registryCount = count;
    out.registryComplete = true;
    out.boxes.reserve(count);

    for (std::uint32_t index = 0;
         index < count;
         ++index) {
        const SlotReadResult slot =
            readLiveVehicle(
                mwsdk::mw05::vehicle_at(index)
            );

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
            // A live-list slot itself could not be trusted. Do not claim full
            // fleet coverage for this sample.
            out.registryComplete = false;
        }
    }

    return out;
}

} // namespace frr::game
