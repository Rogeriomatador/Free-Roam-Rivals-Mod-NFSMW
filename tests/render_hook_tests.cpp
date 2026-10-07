#include "game/RenderObservationHook.h"
#include <windows.h>
#include <d3d9.h>
#include <array>
#include <cstdlib>
#include <iostream>

namespace {
struct FakeDevice { void** vtable; LONG endCalls = 0; LONG presentCalls = 0; };
LONG delivered = 0;
const RECT* seenSource = nullptr;
const RECT* seenDest = nullptr;
HWND seenWindow = nullptr;
const RGNDATA* seenDirty = nullptr;
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
void observe(void*) { InterlockedIncrement(&delivered); }
__declspec(noinline) HRESULT WINAPI endA(IDirect3DDevice9* self) {
    auto* device = reinterpret_cast<FakeDevice*>(self);
    InterlockedIncrement(&device->endCalls);
    return device->endCalls > 0 ? 101 : 102;
}
__declspec(noinline) HRESULT WINAPI endB(IDirect3DDevice9* self) {
    auto* device = reinterpret_cast<FakeDevice*>(self);
    InterlockedIncrement(&device->endCalls);
    return device->endCalls > 0 ? 201 : 202;
}
__declspec(noinline) HRESULT WINAPI present(IDirect3DDevice9* self,
    const RECT* source, const RECT* dest, HWND window, const RGNDATA* dirty) {
    auto* device = reinterpret_cast<FakeDevice*>(self);
    InterlockedIncrement(&device->presentCalls);
    seenSource = source; seenDest = dest; seenWindow = window; seenDirty = dirty;
    return device->presentCalls > 0 ? 301 : 302;
}
using End = HRESULT (WINAPI*)(IDirect3DDevice9*);
using Present = HRESULT (WINAPI*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
}
int main() {
    using frr::game::RenderObservationHook;
    std::array<void*, 43> tableA{}, tableB{};
    tableA[42] = reinterpret_cast<void*>(&endA);
    tableB[42] = reinterpret_cast<void*>(&endB);
    tableA[17] = tableB[17] = reinterpret_cast<void*>(&present);
    FakeDevice a{tableA.data()}, b{tableB.data()};
    auto* selfA = reinterpret_cast<IDirect3DDevice9*>(&a);
    auto* selfB = reinterpret_cast<IDirect3DDevice9*>(&b);
    // Cache the method before installation: an inline target hook must catch
    // this call even though no engine vtable slot is replaced.
    End volatile cachedA = reinterpret_cast<End>(tableA[42]);
    End volatile cachedB = reinterpret_cast<End>(tableB[42]);
    Present volatile cachedPresent = reinterpret_cast<Present>(tableA[17]);
    require(RenderObservationHook::attachForTest(&a, observe), "native hooks install on guarded COM-like device");
    require(tableA[42] == reinterpret_cast<void*>(&endA), "engine vtable is unchanged");
    require(cachedA(selfA) == 101 && a.endCalls == 1 && delivered == 1,
            "cached EndScene pointer reaches observation and original HRESULT");
    RECT source{1,2,3,4}, dest{5,6,7,8};
    RGNDATA dirty{};
    const auto window = reinterpret_cast<HWND>(static_cast<std::uintptr_t>(123));
    require(cachedPresent(selfA, &source, &dest, window, &dirty) == 301,
            "Present HRESULT is preserved");
    require(seenSource == &source && seenDest == &dest && seenWindow == window && seenDirty == &dirty,
            "Present forwards every argument unchanged");
    require(delivered == 1 && a.presentCalls == 1, "same-frame Present chains but does not double sample");
    require(RenderObservationHook::attachForTest(&b, observe), "replacement device supports new implementation");
    cachedPresent(selfB, nullptr, nullptr, nullptr, nullptr);
    require(delivered == 2, "replacement device can sample from Present first");
    require(cachedB(selfB) == 201 && b.endCalls == 1 && delivered == 3,
            "different EndScene implementation retains its own trampoline");
    require(cachedA(selfA) == 101 && a.endCalls == 2 && delivered == 3,
            "old device chains its own original without driving new sampling");
    const auto snapshot = RenderObservationHook::snapshot();
    require(snapshot.endSceneInstalled && snapshot.presentInstalled && snapshot.endSceneCalls == 3 && snapshot.presentCalls == 2,
            "health distinguishes installed targets and raw callback delivery");
    require(!RenderObservationHook::attachForTest(reinterpret_cast<void*>(1), observe),
            "unreadable device fails closed");
    RenderObservationHook::detachForTest();
    require(cachedA(selfA) == 101 && a.endCalls == 3 && delivered == 3,
            "removal restores cached original entry without callbacks");
    std::cout << "Native render hook tests passed\n";
}
