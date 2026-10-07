#include "WorldCollisionProbe.h"

#include <windows.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>

namespace frr::game {
namespace {

// NFSPluginSDK's MW05 UMath vectors expose logical x/y/z fields but their
// physical memory order is y,z,x. CheckHitWorld receives UMath::Vector4 bytes,
// so this raw ABI mirror must preserve that storage order exactly.
struct RawVector4 {
    float y;
    float z;
    float x;
    float w;
};

#pragma pack(push, 1)
struct RawWorldCollisionInfo {
    RawVector4 collidePoint;        // 0x00
    RawVector4 normal;              // 0x10
    std::uint8_t barrierEntry[0x28];// 0x20
    std::uint32_t worldObject;      // 0x48
    float distance;                 // 0x4C
    std::uint8_t animated;          // 0x50
    std::uint8_t type;              // 0x51
    std::uint16_t pad;              // 0x52
    std::uint32_t collisionInstance;// 0x54
};
#pragma pack(pop)

struct RawWCollisionMgr {
    std::uint32_t surfaceExclusionMask;
    std::uint32_t primitiveMask;
    std::uint8_t reserved[0x50];
};

static_assert(sizeof(RawVector4) == 0x10);
static_assert(offsetof(RawVector4, y) == 0x00);
static_assert(offsetof(RawVector4, z) == 0x04);
static_assert(offsetof(RawVector4, x) == 0x08);
static_assert(offsetof(RawVector4, w) == 0x0C);

static_assert(
    sizeof(RawWorldCollisionInfo) == 0x58,
    "MW05 WorldCollisionInfo layout must be 0x58 bytes"
);

static_assert(
    offsetof(RawWorldCollisionInfo, distance) == 0x4C,
    "MW05 WorldCollisionInfo::distance offset mismatch"
);

static_assert(
    offsetof(RawWorldCollisionInfo, type) == 0x51,
    "MW05 WorldCollisionInfo::type offset mismatch"
);

static_assert(
    sizeof(RawWCollisionMgr) == 0x58,
    "Conservative WCollisionMgr backing storage must be 0x58 bytes"
);

bool finiteVector(
    const frr::domain::SpatialVector3& value
) {
    return
        std::isfinite(value.x) &&
        std::isfinite(value.y) &&
        std::isfinite(value.z);
}

RawVector4 rawVector(
    const frr::domain::SpatialVector3& value
) {
    RawVector4 out{};
    out.x = value.x;
    out.y = value.y;
    out.z = value.z;
    out.w = 1.0f;
    return out;
}

bool executableAddress(
    std::uintptr_t address
) {
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

    const DWORD protection =
        mbi.Protect & 0xFFu;

    return
        protection == PAGE_EXECUTE ||
        protection == PAGE_EXECUTE_READ ||
        protection == PAGE_EXECUTE_READWRITE ||
        protection == PAGE_EXECUTE_WRITECOPY;
}

frr::domain::WorldCollisionSample callCheckHitWorld(
    const frr::domain::SpatialVector3& from,
    const frr::domain::SpatialVector3& to,
    std::uint32_t primitiveMask
) {
    frr::domain::WorldCollisionSample out{};

    if (!finiteVector(from) ||
        !finiteVector(to) ||
        !executableAddress(
            WorldCollisionProbe::kCheckHitWorldAddress
        )) {
        return out;
    }

    out.callAvailable = true;

    RawWCollisionMgr manager{};
    manager.surfaceExclusionMask = 0;
    manager.primitiveMask = 3;

    alignas(16) RawVector4 segment[2] = {
        rawVector(from),
        rawVector(to)
    };

    alignas(16) RawWorldCollisionInfo info{};

#if defined(_MSC_VER)
    __try {
#endif
        using CheckHitWorldFn =
            int(__thiscall*)(
                RawWCollisionMgr*,
                const RawVector4*,
                RawWorldCollisionInfo*,
                std::uint32_t
            );

        const auto fn =
            reinterpret_cast<CheckHitWorldFn>(
                WorldCollisionProbe::
                    kCheckHitWorldAddress
            );

        const int result =
            fn(
                &manager,
                segment,
                &info,
                primitiveMask
            );

        out.callCompleted = true;
        out.hit =
            result != 0 &&
            info.type != 0;
        out.hitType = info.type;

        out.hitPoint = {
            info.collidePoint.x,
            info.collidePoint.y,
            info.collidePoint.z
        };

        out.normal = {
            info.normal.x,
            info.normal.y,
            info.normal.z
        };

        out.engineDistance = info.distance;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        out.callCompleted = false;
        out.hit = false;
        out.hitType = 0;
    }
#endif

    return out;
}

} // namespace

bool WorldCollisionProbe::addressAvailable() {
    return executableAddress(
        kCheckHitWorldAddress
    );
}

frr::domain::WorldCollisionSample
WorldCollisionProbe::sampleGround(
    const frr::domain::SpatialVector3& point
) {
    if (!finiteVector(point)) {
        return {};
    }

    const frr::domain::SpatialVector3 from{
        point.x,
        point.y - 2.0f,
        point.z
    };

    const frr::domain::SpatialVector3 to{
        point.x,
        point.y + 1000.0f,
        point.z
    };

    return callCheckHitWorld(
        from,
        to,
        1u
    );
}

frr::domain::WorldCollisionSample
WorldCollisionProbe::sampleWorldOcclusion(
    const frr::domain::SpatialVector3& from,
    const frr::domain::SpatialVector3& to
) {
    return callCheckHitWorld(
        from,
        to,
        3u
    );
}

} // namespace frr::game
