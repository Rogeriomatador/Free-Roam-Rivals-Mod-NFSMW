#pragma once
#include <cstdint>

namespace frr::domain {
// The target's 0x6ED260 routine uses limits 0x34/0x40 and can kill a
// distant vehicle above the limit. Never invoke that eviction branch
// during a mod construction. The counter is physics instances, not cars.
constexpr bool nativeFactoryCapacitySafe(bool readable, std::uint32_t count,
    bool extendedLimit) {
    return readable && count < (extendedLimit ? 0x40u : 0x34u);
}
}
