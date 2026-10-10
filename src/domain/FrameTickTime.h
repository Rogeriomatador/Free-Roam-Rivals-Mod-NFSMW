#pragma once
#include <cstdint>
namespace frr::domain {
// Exact supported target: 0x663D3A FILD int32 argument, then multiply
// 0x890980 (1/65536) and 0x890D60 (0.001f). Stack word forwarded unchanged.
inline float frameTickSeconds(std::int32_t fixedMilliseconds) {
    return static_cast<float>(fixedMilliseconds)*(1.0f/65536.0f)*0.001f;
}
}
