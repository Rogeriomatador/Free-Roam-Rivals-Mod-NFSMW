#include "domain/NativeFactorySafety.h"
#include <cstdio>
#include <initializer_list>
#include <limits>

int main() {
    using frr::domain::nativeFactoryCapacitySafe;
    // Exhaust both native thresholds, including equality (the target returns
    // false there), overflow values and unreadable counters.
    for (bool extended : {false, true}) {
        const unsigned limit = extended ? 64u : 52u;
        for (unsigned count = 0; count <= 128; ++count) {
            if (nativeFactoryCapacitySafe(true, count, extended) != (count < limit)) return 1;
            if (nativeFactoryCapacitySafe(false, count, extended)) return 2;
        }
        if (nativeFactoryCapacitySafe(true, std::numeric_limits<unsigned>::max(), extended)) return 3;
    }
    std::puts("Native factory capacity guard: both boundaries and unreadable/overflow counters passed; no engine calls");
    return 0;
}
