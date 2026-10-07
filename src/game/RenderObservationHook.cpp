#include "RenderObservationHook.h"
#include "../domain/RenderObservation.h"
#include "../core/Log.h"
#include <windows.h>
#include <d3d9.h>
#include <MinHook.h>
#include <array>
#include <atomic>
#include <sstream>
#include <vector>

namespace frr::game {
namespace {
using EndSceneFn = HRESULT (WINAPI*)(IDirect3DDevice9*);
using PresentFn = HRESULT (WINAPI*)(IDirect3DDevice9*, const RECT*, const RECT*, HWND, const RGNDATA*);
constexpr std::uintptr_t kSdkDeviceGlobal = 0x00982BDCu;
constexpr unsigned kEndSceneSlot = 42, kPresentSlot = 17;
constexpr std::size_t kTargetLimit = 4;
struct HookRecord {
    std::uintptr_t target = 0; // installer thread only
    std::atomic<void*> original{nullptr};
    bool enabled = false;
};
std::array<HookRecord, kTargetLimit> g_endSceneHooks{}, g_presentHooks{};
std::atomic<RenderObservationHook::Callback> g_callback{nullptr};
std::atomic<bool> g_started{false}, g_endSceneInstalled{false}, g_presentInstalled{false};
std::atomic<std::uintptr_t> g_deviceGlobal{0}, g_device{0}, g_vtable{0};
std::atomic<std::uint64_t> g_endSceneCalls{0}, g_presentCalls{0};
std::atomic_flag g_delivering = ATOMIC_FLAG_INIT;
domain::RenderSignalRouter g_router{}; // guarded by g_delivering

bool copyMemory(std::uintptr_t address, void* output, std::size_t size) {
    SIZE_T copied = 0;
    return address != 0 && ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address),
        output, size, &copied) && copied == size;
}
bool executable(std::uintptr_t address) {
    MEMORY_BASIC_INFORMATION info{};
    if (!address || !VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) ||
        info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
    const DWORD protection = info.Protect & 0xff;
    return protection == PAGE_EXECUTE || protection == PAGE_EXECUTE_READ ||
        protection == PAGE_EXECUTE_READWRITE || protection == PAGE_EXECUTE_WRITECOPY;
}
void deliver(domain::RenderSignal signal, IDirect3DDevice9* self) {
    if (signal == domain::RenderSignal::EndScene) ++g_endSceneCalls;
    else ++g_presentCalls;
    const auto identity = reinterpret_cast<std::uintptr_t>(self);
    if (identity != g_device.load() || g_delivering.test_and_set(std::memory_order_acquire)) return;
    if (g_router.accept(signal, identity, GetTickCount64())) {
        if (const auto callback = g_callback.load()) callback(self);
    }
    g_delivering.clear(std::memory_order_release);
}
template <std::size_t I> HRESULT WINAPI endSceneDetour(IDirect3DDevice9* self) {
    deliver(domain::RenderSignal::EndScene, self);
    const auto original = reinterpret_cast<EndSceneFn>(g_endSceneHooks[I].original.load());
    return original ? original(self) : D3DERR_INVALIDCALL;
}
template <std::size_t I> HRESULT WINAPI presentDetour(
    IDirect3DDevice9* self, const RECT* source, const RECT* dest, HWND window, const RGNDATA* dirty) {
    deliver(domain::RenderSignal::Present, self);
    const auto original = reinterpret_cast<PresentFn>(g_presentHooks[I].original.load());
    return original ? original(self, source, dest, window, dirty) : D3DERR_INVALIDCALL;
}
const std::array<EndSceneFn, kTargetLimit> kEndSceneDetours{
    endSceneDetour<0>, endSceneDetour<1>, endSceneDetour<2>, endSceneDetour<3>};
const std::array<PresentFn, kTargetLimit> kPresentDetours{
    presentDetour<0>, presentDetour<1>, presentDetour<2>, presentDetour<3>};

bool hookTarget(std::array<HookRecord, kTargetLimit>& records,
    std::uintptr_t target, bool endScene) {
    const auto& other = endScene ? g_presentHooks : g_endSceneHooks;
    for (const auto& record : other) if (record.target == target && record.enabled) return false;
    for (auto& record : records) if (record.target == target) return record.enabled;
    for (std::size_t i = 0; i < records.size(); ++i) {
        auto& record = records[i];
        if (record.target != 0) continue;
        void* original = nullptr;
        void* detour = endScene ? reinterpret_cast<void*>(kEndSceneDetours[i]) :
            reinterpret_cast<void*>(kPresentDetours[i]);
        const auto status = MH_CreateHook(reinterpret_cast<void*>(target), detour, &original);
        if (status != MH_OK) {
            std::ostringstream line;
            line << "Render method hook creation failed: method=" << (endScene ? "EndScene" : "Present")
                 << " status=" << MH_StatusToString(status);
            Log::instance().warn(line.str());
            return false;
        }
        // Publish the trampoline BEFORE enabling the detour; cached engine
        // method pointers can reach it as soon as MinHook enables the hook.
        record.original.store(original);
        record.target = target;
        const auto enabled = MH_EnableHook(reinterpret_cast<void*>(target));
        record.enabled = enabled == MH_OK;
        if (!record.enabled) {
            MH_RemoveHook(reinterpret_cast<void*>(target));
            record.target = 0;
            record.original.store(nullptr);
        }
        std::ostringstream line;
        line << "Render method hook: method=" << (endScene ? "EndScene" : "Present")
             << " target=0x" << std::hex << target << std::dec
             << " enabled=" << (record.enabled ? 1 : 0)
             << " status=" << MH_StatusToString(enabled);
        HMODULE module = nullptr;
        char path[MAX_PATH]{};
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCSTR>(target), &module) && GetModuleFileNameA(module, path, MAX_PATH)) {
            line << " module=" << path;
        }
        Log::instance().info(line.str());
        return record.enabled;
    }
    return false; // bounded target history; never reuse a live trampoline
}

bool attach(std::uintptr_t device) {
    std::uintptr_t vtable = 0, endScene = 0, present = 0;
    if (!copyMemory(device, &vtable, sizeof(vtable)) ||
        !copyMemory(vtable + kEndSceneSlot * sizeof(void*), &endScene, sizeof(endScene)) ||
        !copyMemory(vtable + kPresentSlot * sizeof(void*), &present, sizeof(present)) ||
        !executable(endScene) || !executable(present) || endScene == present) return false;
    const auto initialized = MH_Initialize();
    if (initialized != MH_OK && initialized != MH_ERROR_ALREADY_INITIALIZED) return false;
    const bool es = hookTarget(g_endSceneHooks, endScene, true);
    const bool ps = hookTarget(g_presentHooks, present, false);
    g_endSceneInstalled.store(es);
    g_presentInstalled.store(ps);
    g_vtable.store(vtable);
    g_device.store(device);
    return es || ps;
}

std::uintptr_t resolveGlobal() {
    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    IMAGE_DOS_HEADER dos{};
    IMAGE_NT_HEADERS32 nt{};
    if (!copyMemory(base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
        dos.e_lfanew <= 0 || dos.e_lfanew > 1024 * 1024 ||
        !copyMemory(base + dos.e_lfanew, &nt, sizeof(nt)) || nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        nt.OptionalHeader.SizeOfImage > 32 * 1024 * 1024 || nt.OptionalHeader.SizeOfImage < sizeof(nt) ||
        nt.FileHeader.NumberOfSections > 96) return 0;
    const auto sectionBase = base + dos.e_lfanew + offsetof(IMAGE_NT_HEADERS32, OptionalHeader) +
        nt.FileHeader.SizeOfOptionalHeader;
    domain::DeviceGlobalResolution combined{};
    for (unsigned i = 0; i < nt.FileHeader.NumberOfSections; ++i) {
        IMAGE_SECTION_HEADER section{};
        if (!copyMemory(sectionBase + i * sizeof(section), &section, sizeof(section))) return 0;
        if (!(section.Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        const auto size = section.Misc.VirtualSize;
        if (section.VirtualAddress > nt.OptionalHeader.SizeOfImage ||
            size > nt.OptionalHeader.SizeOfImage - section.VirtualAddress) return 0;
        std::vector<std::uint8_t> code(size);
        if (!copyMemory(base + section.VirtualAddress, code.data(), code.size())) return 0;
        const auto found = domain::resolveRenderDeviceGlobal(code, base, nt.OptionalHeader.SizeOfImage);
        if (found.match == domain::DeviceGlobalMatch::Ambiguous ||
            (combined.match == domain::DeviceGlobalMatch::Unique && found.match == domain::DeviceGlobalMatch::Unique &&
             found.address != combined.address)) {
            Log::instance().warn("Ambiguous MW05 render-device signature; render observation remains blocked.");
            return 0;
        }
        if (found.match == domain::DeviceGlobalMatch::Unique) combined = found;
    }
    const auto address = combined.match == domain::DeviceGlobalMatch::Unique ? combined.address : kSdkDeviceGlobal;
    std::ostringstream line;
    line << "Render device global resolved: source=" <<
        (combined.match == domain::DeviceGlobalMatch::Unique ? "MW05_Reset_signature" : "pinned_SDK_fallback")
         << " address=0x" << std::hex << address;
    Log::instance().info(line.str());
    return address;
}

DWORD WINAPI worker(LPVOID) {
    const auto global = resolveGlobal();
    g_deviceGlobal.store(global);
    if (!global) { Log::instance().warn("Render device discovery failed; no render hook installed."); return 0; }
    std::uintptr_t lastDevice = 0, lastVtable = 0, lastEndScene = 0, lastPresent = 0;
    bool haveInstalled = false;
    std::uint64_t nextAttempt = 0;
    for (;;) {
        std::uintptr_t device = 0, vtable = 0, es = 0, ps = 0;
        const bool valid = copyMemory(global, &device, sizeof(device)) && device != 0 &&
            copyMemory(device, &vtable, sizeof(vtable)) &&
            copyMemory(vtable + kEndSceneSlot * sizeof(void*), &es, sizeof(es)) &&
            copyMemory(vtable + kPresentSlot * sizeof(void*), &ps, sizeof(ps)) && executable(es) && executable(ps);
        if (valid && (device != lastDevice || vtable != lastVtable || es != lastEndScene || ps != lastPresent ||
            (!haveInstalled && GetTickCount64() >= nextAttempt))) {
            haveInstalled = attach(device);
            lastDevice = device; lastVtable = vtable; lastEndScene = es; lastPresent = ps;
            nextAttempt = GetTickCount64() + 5000;
            std::ostringstream line;
            line << "Render device observation: device=0x" << std::hex << device << " vtable=0x" << vtable
                 << std::dec << " hooked=" << (haveInstalled ? 1 : 0);
            Log::instance().info(line.str());
        }
        if (!valid) {
            g_device.store(0);
            g_endSceneInstalled.store(false);
            g_presentInstalled.store(false);
            haveInstalled = false;
        }
        Sleep(250);
    }
}
} // namespace

bool RenderObservationHook::install(Callback callback) {
    if (!callback) return false;
    g_callback.store(callback);
    if (g_started.exchange(true)) return true;
    const auto thread = CreateThread(nullptr, 0, worker, nullptr, 0, nullptr);
    if (!thread) { g_started.store(false); return false; }
    CloseHandle(thread);
    return true;
}
RenderHookSnapshot RenderObservationHook::snapshot() {
    return {g_started.load(), g_endSceneInstalled.load(), g_presentInstalled.load(),
        g_deviceGlobal.load(), g_device.load(), g_vtable.load(), g_endSceneCalls.load(), g_presentCalls.load()};
}
#ifdef FRR_RENDER_HOOK_TESTING
bool RenderObservationHook::attachForTest(void* device, Callback callback) {
    g_callback.store(callback);
    return attach(reinterpret_cast<std::uintptr_t>(device));
}
void RenderObservationHook::detachForTest() {
    for (auto* records : {&g_endSceneHooks, &g_presentHooks}) for (auto& record : *records) {
        if (record.enabled) {
            MH_DisableHook(reinterpret_cast<void*>(record.target));
            MH_RemoveHook(reinterpret_cast<void*>(record.target));
        }
        record.enabled = false; record.target = 0; record.original.store(nullptr);
    }
    g_device.store(0);
}
#endif
} // namespace frr::game
