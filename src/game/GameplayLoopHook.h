#pragma once
#include <cstdint>

namespace frr::game {
struct GameplayLoopSnapshot {
    bool installed = false;
    bool sourceVerified = false;
    bool threadConsistent = false;
    std::uintptr_t callSite = 0;
    std::uintptr_t target = 0;
    std::uint64_t entered = 0;
    std::uint64_t completed = 0;
    std::uint32_t threadId = 0;
};
class GameplayLoopHook {
public:
    // Seconds converted from the native signed fixed-millisecond stack word.
    using Callback = void (*)(float);
    static bool install(Callback before, Callback after);
    static GameplayLoopSnapshot snapshot();
    // Current call boundary only; callers must separately check provenance.
    static bool isInAfterCallback();
#ifdef FRR_GAMEPLAY_HOOK_TESTING
    static bool attachForTest(void* target, Callback before, Callback after);
    static void detachForTest();
#endif
};
} // namespace frr::game
