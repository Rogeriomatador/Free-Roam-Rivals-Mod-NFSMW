#include "GameplayLoopEvidence.h"
#include <cstring>
#include <limits>

namespace frr::domain {
GameplayLoopResolution resolveGameplayLoop(std::span<const std::uint8_t> code,
    std::uintptr_t codeAddress, std::uintptr_t imageBase, std::size_t imageSize) {
    GameplayLoopResolution out{};
    constexpr std::size_t expectedRva = 0x263D30;
    if (code.size() < 19 || imageSize <= expectedRva || codeAddress < imageBase ||
        codeAddress - imageBase > imageSize || code.size() > imageSize - (codeAddress - imageBase) ||
        imageBase > std::numeric_limits<std::uintptr_t>::max() - imageSize) return out;
    for (std::size_t i = 0; i <= code.size() - 19; ++i) {
        const auto* p = code.data() + i;
        if (p[0] != 0xE8 || p[5] != 0xA0 || p[10] != 0x83 || p[11] != 0xC4 ||
            p[12] != 4 || p[13] != 0x84 || p[14] != 0xC0 || p[15] != 0x74 ||
            p[17] != 0xC6 || p[18] != 5) continue;
        std::uint32_t global = 0;
        std::int32_t displacement = 0;
        std::memcpy(&global, p + 6, 4);
        std::memcpy(&displacement, p + 1, 4);
        if (global < imageBase || global - imageBase >= imageSize) continue;
        const auto site = codeAddress + i;
        const auto destination = static_cast<std::int64_t>(site) + 5 + displacement;
        if (out.match != GameplayLoopMatch::Missing) return {GameplayLoopMatch::Ambiguous, 0, 0};
        if (destination != static_cast<std::int64_t>(imageBase + expectedRva)) {
            out = {GameplayLoopMatch::UnexpectedTarget, site,
                destination > 0 && destination <= UINT32_MAX ? static_cast<std::uintptr_t>(destination) : 0};
        } else out = {GameplayLoopMatch::Unique, site, static_cast<std::uintptr_t>(destination)};
    }
    return out;
}
} // namespace frr::domain
