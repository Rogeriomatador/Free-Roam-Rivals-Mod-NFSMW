#include "domain/NativeSearchWindow.h"
#include <vector>
#include <cstdio>

int main() {
    using namespace frr::domain;
    // Every source group and every candidate in each group must eventually
    // be examined, including when population/budget share a common divisor.
    for (std::size_t population : {1u, 3u, 16u, 32u, 48u}) {
    for (std::size_t groups = 1; groups <= 32; ++groups) {
        std::size_t sourceCursor = 0;
        std::vector<std::size_t> candidateCursors(groups);
        std::vector<std::vector<bool>> seen(groups, std::vector<bool>(population));
        for (std::size_t tick = 0; tick < groups * ((population+3)/4); ++tick) {
            const auto group = nextNativeSearchWindow(groups, 1, sourceCursor).index(0);
            const auto candidates = nextNativeSearchWindow(population, 4, candidateCursors[group]);
            if (candidates.count > 4) return 1;
            for (std::size_t i = 0; i < candidates.count; ++i) seen[group][candidates.index(i)] = true;
        }
        for (const auto& group : seen) for (bool visited : group) if (!visited) return 2;
    }
    }
    std::size_t cursor = 31;
    auto shrinking = nextNativeSearchWindow(3, 4, cursor);
    if (shrinking.count != 3 || shrinking.index(0) >= 3 || shrinking.index(3) != 3) return 3;
    if (nextNativeSearchWindow(0, 4, cursor).count || cursor) return 4;
    cursor = 99;
    if (nextNativeSearchWindow(10, 0, cursor).count || cursor) return 5;
    std::puts("Native candidate search coverage tests passed (bounded work, isolated group cursors, shrinking/empty populations).");
    return 0;
}
