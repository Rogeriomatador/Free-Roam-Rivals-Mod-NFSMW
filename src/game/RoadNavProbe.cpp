#include "RoadNavProbe.h"

#include <windows.h>

#include <NFSPluginSDK/Game.MW05/MW05.h>
#include <NFSPluginSDK/Game.MW05/Extensions.h>

#include <cmath>
#include <cstddef>
#include <cstdint>

namespace frr::game {
namespace {

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

    if (mbi.State != MEM_COMMIT ||
        (mbi.Protect & PAGE_GUARD) != 0 ||
        (mbi.Protect & PAGE_NOACCESS) != 0) {
        return false;
    }

    const auto begin =
        reinterpret_cast<std::uintptr_t>(mbi.BaseAddress);
    const auto end = begin + mbi.RegionSize;

    return address >= begin &&
        address <= end &&
        size <= (end - address);
}

template <typename V>
RoadVectorProbe copyVector(const V& value) {
    RoadVectorProbe out{};
    out.x = value.x;
    out.y = value.y;
    out.z = value.z;
    out.finite =
        std::isfinite(out.x) &&
        std::isfinite(out.y) &&
        std::isfinite(out.z);
    return out;
}

float distance(
    const RoadVectorProbe& a,
    const RoadVectorProbe& b
) {
    if (!a.finite || !b.finite) {
        return 0.0f;
    }

    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;

    const float result = std::sqrt(
        dx * dx + dy * dy + dz * dz
    );

    return std::isfinite(result) ? result : 0.0f;
}

RoadNavPointProbe probeRoad(
    NFSPluginSDK::MW05::WRoadNav* road
) {
    RoadNavPointProbe out{};

    if (!road ||
        !isReadable(
            reinterpret_cast<std::uintptr_t>(road),
            sizeof(NFSPluginSDK::MW05::WRoadNav))) {
        return out;
    }

#if defined(_MSC_VER)
    __try {
#endif
        out.address =
            reinterpret_cast<std::uintptr_t>(road);
        out.valid = road->fValid;
        out.deadEnd = road->fDeadEnd != 0;
        out.segmentIndex =
            static_cast<std::int32_t>(road->fSegmentInd);
        out.laneIndex =
            static_cast<std::int32_t>(road->fLaneInd);
        out.curvature = road->fCurvature;

        out.position = copyVector(road->fPosition);
        out.forward = copyVector(road->fForwardVector);
        out.leftPosition = copyVector(road->fLeftPosition);
        out.rightPosition = copyVector(road->fRightPosition);
        out.startPosition = copyVector(road->fStartPos);
        out.endPosition = copyVector(road->fEndPos);

        out.roadWidthMeters = distance(
            out.leftPosition,
            out.rightPosition
        );

        out.segmentSpanMeters = distance(
            out.startPosition,
            out.endPosition
        );

        const bool scalarSane =
            std::isfinite(out.curvature) &&
            std::isfinite(out.roadWidthMeters) &&
            std::isfinite(out.segmentSpanMeters) &&
            out.roadWidthMeters >= 0.0f &&
            out.roadWidthMeters < 100.0f &&
            out.segmentSpanMeters >= 0.0f &&
            out.segmentSpanMeters < 5000.0f;

        out.available =
            scalarSane &&
            out.position.finite &&
            out.forward.finite &&
            out.leftPosition.finite &&
            out.rightPosition.finite;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out = RoadNavPointProbe{};
    }
#endif

    return out;
}

} // namespace

PlayerRoadNavigationProbe RoadNavProbe::sample(
    std::uintptr_t playerPVehicle
) {
    PlayerRoadNavigationProbe out{};

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

        IVehicleAI* ai = player->GetAIVehiclePtr();
        if (!ai) {
            return out;
        }

        out.playerAiAvailable = true;
        out.playerAi =
            reinterpret_cast<std::uintptr_t>(ai);

        out.current = probeRoad(ai->GetCurrentRoad());
        out.future = probeRoad(ai->GetFutureRoad());

        out.currentToFutureMeters = distance(
            out.current.position,
            out.future.position
        );

        out.available =
            out.current.available ||
            out.future.available;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out = PlayerRoadNavigationProbe{};
    }
#endif

    return out;
}

} // namespace frr::game
