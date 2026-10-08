#include "domain/NativeRoadSeed.h"
#include <cstdio>
#include <limits>

int main() {
    using namespace frr::domain;
    NativeRoadSeed seed{321, 1, 3, 0.625f, -1.5f, true};
    NativeRoadSeedBytes encoded{};
    if (!encodeNativeRoadSeed(seed, encoded)) return 1;
    std::int16_t segment = 0;
    float time = 0, offset = 0;
    std::memcpy(&segment, encoded.bytes.data() + 0x8E, sizeof(segment));
    std::memcpy(&time, encoded.bytes.data() + 0x90, sizeof(time));
    std::memcpy(&offset, encoded.bytes.data() + 0x2C4, sizeof(offset));
    if (segment != 321 || time != 0.625f || offset != -1.5f ||
        encoded.bytes[0x8C] != 1 || encoded.bytes[0x2C1] != 3 || encoded.bytes[0x50] != 1) return 2;
    // All fields that could retain borrowed cookie/AI/spline pointers remain
    // zero; only the six native scalar reads may have non-zero bytes.
    for (unsigned i = 0; i < encoded.bytes.size(); ++i) {
        bool scalar = i == 0x50 || i == 0x8C || (i >= 0x8E && i < 0x94) ||
            i == 0x2C1 || i >= 0x2C4;
        if (!scalar && encoded.bytes[i]) return 3;
    }
    NativeRoadSeed invalid[] = {
        {-1, 1, 0, 0, 0, true}, {32768, 1, 0, 0, 0, true},
        {1, 2, 0, 0, 0, true}, {1, 1, -1, 0, 0, true}, {1, 1, 16, 0, 0, true},
        {1, 1, 0, -0.01f, 0, true}, {1, 1, 0, 1.01f, 0, true},
        {1, 1, 0, std::numeric_limits<float>::quiet_NaN(), 0, true},
        {1, 1, 0, 0, std::numeric_limits<float>::infinity(), true}, {1, 1, 0, 0, 21, true},
        {1, 1, 0, 0, 0, false}
    };
    for (const auto& bad : invalid) {
        if (encodeNativeRoadSeed(bad, encoded)) return 4;
        for (auto byte : encoded.bytes) if (byte) return 5;
    }
    std::puts("Owned native road seed: scalar round-trip, pointer exclusion, malformed and non-finite inputs passed; no engine calls");
    return 0;
}
