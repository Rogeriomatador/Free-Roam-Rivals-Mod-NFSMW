#include "ChallengeInputProbe.h"

#include "../domain/ChallengeInput.h"

#include <windows.h>

#include <atomic>
#include <cstdint>

namespace frr::game {
namespace {

frr::domain::ChallengeInputEdge g_edge{};
unsigned g_fallbackVirtualKey = 0x47u;

constexpr std::uint32_t kMaxPendingPresses = 16u;

std::atomic<std::uint32_t> g_pendingPresses{0};
std::atomic<std::uint32_t> g_pendingModeToggles{0};
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
    g_pendingModeToggles.store(0,std::memory_order_relaxed);
    g_totalPresses.store(
        0,
        std::memory_order_relaxed
    );
}

void ChallengeInputProbe::onPoll() {
    bool down = false;

    DWORD foregroundProcess = 0;
    GetWindowThreadProcessId(GetForegroundWindow(), &foregroundProcess);
    if (foregroundProcess == GetCurrentProcessId() && validVirtualKey(g_fallbackVirtualKey)) {
        const SHORT state = GetAsyncKeyState(
            static_cast<int>(g_fallbackVirtualKey)
        );
        down = (state & 0x8000) != 0;
    }

    if (!g_edge.update(down)) {
        return;
    }

    if ((GetAsyncKeyState(VK_CONTROL)&0x8000)!=0 &&
        (GetAsyncKeyState(VK_SHIFT)&0x8000)!=0) {
        const auto current=g_pendingModeToggles.load(std::memory_order_relaxed);
        if(current<kMaxPendingPresses) g_pendingModeToggles.fetch_add(1,std::memory_order_release);
        return;
    }
    g_totalPresses.fetch_add(
        1,
        std::memory_order_relaxed
    );

    std::uint32_t current =
        g_pendingPresses.load(
            std::memory_order_acquire
        );

    while (current < kMaxPendingPresses) {
        if (g_pendingPresses.compare_exchange_weak(
                current,
                current + 1u,
                std::memory_order_acq_rel,
                std::memory_order_acquire)) {
            break;
        }
    }
}

bool ChallengeInputProbe::consumeModeToggle() {
    auto count=g_pendingModeToggles.load(std::memory_order_acquire);
    while(count>0) {
        if(g_pendingModeToggles.compare_exchange_weak(count,count-1,
            std::memory_order_acq_rel,std::memory_order_acquire)) return true;
    }
    return false;
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

