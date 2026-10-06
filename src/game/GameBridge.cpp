#include "GameBridge.h"

#include <nfsmw_sdk/globals.h>

#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <cstring>

namespace frr::game {
namespace {

constexpr std::uintptr_t kPVehicleInstances = 0x009352B0u;
constexpr std::uintptr_t kPVehicleInstanceStride = 8u;
constexpr std::uint32_t kMaxPVehicleInstancesToProbe = 96u;

bool isReadable(std::uintptr_t address, std::size_t size) {
    if (address == 0 || size == 0) {
        return false;
    }

    MEMORY_BASIC_INFORMATION mbi{};
    if (VirtualQuery(
            reinterpret_cast<const void*>(address),
            &mbi,
            sizeof(mbi)) == 0) {
        return false;
    }

    if (mbi.State != MEM_COMMIT) {
        return false;
    }

    if ((mbi.Protect & PAGE_GUARD) != 0 ||
        (mbi.Protect & PAGE_NOACCESS) != 0) {
        return false;
    }

    const auto regionBegin =
        reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
    const auto regionEnd = regionBegin + mbi.RegionSize;

    if (address < regionBegin || address > regionEnd) {
        return false;
    }

    return size <= (regionEnd - address);
}

template <typename T>
bool readValue(std::uintptr_t address, T& value) {
    if (!isReadable(address, sizeof(T))) {
        return false;
    }

    std::memcpy(
        &value,
        reinterpret_cast<const void*>(address),
        sizeof(T)
    );

    return true;
}

VehicleProbe probeVehicles() {
    VehicleProbe result{};

    for (std::uint32_t i = 0;
         i < kMaxPVehicleInstancesToProbe;
         ++i) {
        const std::uintptr_t entryAddress =
            kPVehicleInstances +
            static_cast<std::uintptr_t>(i) *
                kPVehicleInstanceStride;

        std::uint32_t object32 = 0;
        if (!readValue(entryAddress, object32)) {
            break;
        }

        if (object32 == 0) {
            break;
        }

        const std::uintptr_t object =
            static_cast<std::uintptr_t>(object32);

        std::uint32_t vtable32 = 0;
        if (!readValue(object, vtable32)) {
            ++result.unknownVehicles;
            ++result.totalVehicles;
            continue;
        }

        ++result.totalVehicles;

        if (vtable32 == NFSMW_VTBL_PVehicle_PlayerCar) {
            ++result.playerVehicles;

            if (result.playerVehicle == 0) {
                result.playerVehicle = object;
            }
        } else if (vtable32 == NFSMW_VTBL_PVehicle_AICar) {
            ++result.aiVehicles;
        } else {
            ++result.unknownVehicles;
        }
    }

    return result;
}

} // namespace

RuntimeSnapshot GameBridge::sample() {
    RuntimeSnapshot snapshot{};

    bool inNIS = false;
    if (readValue(NFSMW_GLOBAL_IsInNIS, inNIS)) {
        snapshot.inNIS = inNIS;
    }

    bool fade = false;
    if (readValue(NFSMW_GLOBAL_IsFadeScreenOn, fade)) {
        snapshot.fadeScreen = fade;
    }

    std::uint32_t raceStatus32 = 0;
    if (readValue(
            NFSMW_SINGLETON_GRaceStatus,
            raceStatus32)) {
        snapshot.raceStatus =
            static_cast<std::uintptr_t>(raceStatus32);
    }

    std::uint32_t gameFlow32 = 0;
    if (readValue(
            NFSMW_GLOBAL_TheGameFlowManager,
            gameFlow32)) {
        // Keep this diagnostic-only: current RE research
        // warns this value is not a reliable enum.
        snapshot.gameFlowRaw =
            static_cast<std::uintptr_t>(gameFlow32);
    }

    snapshot.vehicles = probeVehicles();

    if (snapshot.vehicles.playerVehicle == 0) {
        snapshot.mode = WorldProbeMode::NoPlayerVehicle;
    } else if (snapshot.inNIS) {
        snapshot.mode = WorldProbeMode::NIS;
    } else if (snapshot.fadeScreen) {
        snapshot.mode = WorldProbeMode::Transition;
    } else {
        // This is deliberately NOT called FreeRoam yet,
        // because stock races also have a live player car.
        snapshot.mode = WorldProbeMode::DrivingCandidate;
    }

    return snapshot;
}

const char* worldProbeModeName(WorldProbeMode mode) {
    switch (mode) {
        case WorldProbeMode::NoPlayerVehicle:
            return "NoPlayerVehicle";
        case WorldProbeMode::Transition:
            return "Transition";
        case WorldProbeMode::NIS:
            return "NIS";
        case WorldProbeMode::DrivingCandidate:
            return "DrivingCandidate";
    }

    return "Unknown";
}

} // namespace frr::game
