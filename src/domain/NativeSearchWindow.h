#pragma once
#include <algorithm>
#include <cstddef>

namespace frr::domain {
struct NativeSearchWindow {
    std::size_t population = 0, start = 0, count = 0;
    std::size_t index(std::size_t offset) const {
        return population && offset < count ? (start + offset) % population : population;
    }
};
inline NativeSearchWindow nextNativeSearchWindow(std::size_t population,
    std::size_t budget, std::size_t& cursor) {
    if (!population || !budget) { cursor = 0; return {}; }
    NativeSearchWindow out{population, cursor % population, std::min(population, budget)};
    cursor = (out.start + out.count) % population;
    return out;
}
}
