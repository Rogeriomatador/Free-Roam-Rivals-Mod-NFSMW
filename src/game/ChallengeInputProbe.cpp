#include "ChallengeInputProbe.h"

#include "../domain/ChallengeInput.h"

#include <windows.h>

#include <atomic>
#include <cstdint>

namespace frr::game {
namespace {

frr::domain::ChallengeInputEdge g_edge{};
unsigned g_fallbackVirtualKey = 0x47u;

std::atomic<std::uint32_t> g_pendingPresses{0};
std::atomic<std::uint64_t> g_totalPresses{0};

bool validVirtualKey(unsigned key) {
    return key > 0u && key <= 0xFEu;
}

} // namespace

void ChallengeInputProbe::configure(
    const ChallengeInputProbeConfig& config
) {
    g_fallbackVirtualKey =
        validVirtualKey(config.fallbackVirtualKey)
        ? config.fallbackVirtualKey
        : 0u;

    g_edge.reset();
    g_pendingPresses.store(
        0,
        std::memory_order_relaxed
    );
    g_totalPresses.store(
        0,
        std::memory_order_relaxed
    );
}

void ChallengeInputProbe::onPoll() {
    bool down = false;

    if (validVirtualKey(g_fallbackVirtualKey)) {
        const SHORT state = GetAsyncKeyState(
            static_cast<int>(g_fallbackVirtualKey)
        );
        down = (state & 0x8000) != 0;
    }

    if (!g_edge.update(down)) {
        return;
    }

    g_pendingPresses.fetch_add(
        1,
        std::memory_order_release
    );

    g_totalPresses.fetch_add(
        1,
        std::memory_order_relaxed
    );
}

bool ChallengeInputProbe::consumePress() {
    std::uint32_t current =
        g_pendingPresses.load(
            std::memory_order_acquire
        );

    while (current > 0u) {
        if (g_pendingPresses.compare_exchange_weak(
                current,
                current - 1u,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            return true;
        }
    }

    return false;
}

std::uint64_t ChallengeInputProbe::totalPresses() {
    return g_totalPresses.load(
        std::memory_order_relaxed
    );
}

unsigned ChallengeInputProbe::fallbackVirtualKey() {
    return g_fallbackVirtualKey;
}

} // namespace frr::game
