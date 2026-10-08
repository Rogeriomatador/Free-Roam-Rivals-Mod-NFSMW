#pragma once
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace frr::domain {
struct NativeRoadSeed {
    std::int32_t segment = -1;
    std::int32_t node = -1;
    std::int32_t lane = -1;
    float segmentTime = 0.0f;
    float laneOffset = 0.0f;
    bool valid = false;
};
inline bool validNativeRoadSeed(const NativeRoadSeed& seed) {
    return seed.valid && seed.segment >= 0 && seed.segment <= 32767 &&
        seed.node >= 0 && seed.node <= 1 && seed.lane >= 0 && seed.lane <= 15 &&
        std::isfinite(seed.segmentTime) && seed.segmentTime >= 0.0f && seed.segmentTime <= 1.0f &&
        std::isfinite(seed.laneOffset) && std::abs(seed.laneOffset) <= 20.0f;
}
// Not a C++ WRoadNav object. The verified 0x777660 routine reads precisely
// these scalar source fields and calls no virtual/source-object methods.
// All pointer-shaped/unused storage remains zero. Destination navigation is
// the AI's existing, engine-owned, initialized object.
struct alignas(4) NativeRoadSeedBytes {
    std::array<unsigned char, 0x2C8> bytes{};
};
inline bool encodeNativeRoadSeed(const NativeRoadSeed& seed, NativeRoadSeedBytes& out) {
    out = {};
    if (!validNativeRoadSeed(seed)) return false;
    const auto segment = static_cast<std::int16_t>(seed.segment);
    const auto node = static_cast<std::int8_t>(seed.node);
    const auto lane = static_cast<std::int8_t>(seed.lane);
    out.bytes[0x50] = 1;
    std::memcpy(out.bytes.data() + 0x8C, &node, sizeof(node));
    std::memcpy(out.bytes.data() + 0x8E, &segment, sizeof(segment));
    std::memcpy(out.bytes.data() + 0x90, &seed.segmentTime, sizeof(seed.segmentTime));
    std::memcpy(out.bytes.data() + 0x2C1, &lane, sizeof(lane));
    std::memcpy(out.bytes.data() + 0x2C4, &seed.laneOffset, sizeof(seed.laneOffset));
    return true;
}
}
