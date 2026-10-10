#pragma once
#include <cstdint>

namespace frr::game {
struct RenderHookSnapshot {
    bool workerStarted = false;
    bool endSceneInstalled = false;
    bool presentInstalled = false;
    std::uintptr_t deviceGlobal = 0;
    std::uintptr_t device = 0;
    std::uintptr_t vtable = 0;
    std::uint64_t endSceneCalls = 0;
    std::uint64_t presentCalls = 0;
};

class RenderObservationHook {
public:
    using Callback = void (*)(void*);
    static bool install(Callback callback);
    static RenderHookSnapshot snapshot();
    static void setHudCallback(Callback callback);
#ifdef FRR_RENDER_HOOK_TESTING
    static bool attachForTest(void* device, Callback callback);
    static void detachForTest();
#endif
};
} // namespace frr::game
