#include "GameplayLoopHook.h"
#include "../domain/FrameTickTime.h"
#include "../domain/GameplayLoopEvidence.h"
#include "../core/Log.h"
#include <windows.h>
#include <MinHook.h>
#include <atomic>
#include <cwctype>
#include <sstream>
#include <string>
#include <vector>

namespace frr::game {
namespace {
using LoopFn = void (__cdecl*)(std::int32_t);
std::atomic<LoopFn> g_original{nullptr};
GameplayLoopHook::Callback g_before = nullptr, g_after = nullptr;
std::atomic<bool> g_installed{false}, g_verified{false}, g_consistent{true};
std::atomic<std::uint64_t> g_entered{0}, g_completed{0};
std::atomic<DWORD> g_thread{0};
std::atomic<std::uintptr_t> g_site{0}, g_target{0};
thread_local unsigned g_depth = 0;
thread_local bool g_inAfterCallback = false;
struct AfterCallbackScope {
    AfterCallbackScope() { g_inAfterCallback = true; }
    ~AfterCallbackScope() { g_inAfterCallback = false; }
};

void __cdecl detour(std::int32_t tickerDifference) {
    const DWORD thread = GetCurrentThreadId();
    DWORD first = 0;
    g_thread.compare_exchange_strong(first, thread);
    if (g_thread.load() != thread) g_consistent.store(false);
    ++g_entered;
    const bool outer = ++g_depth == 1;
    if (outer && g_consistent.load() && g_before) g_before(domain::frameTickSeconds(tickerDifference));
    // Callback state and trampoline are published before enabling this entry.
    if (const auto original = g_original.load()) original(tickerDifference);
    ++g_completed;
    if (outer && g_consistent.load() && g_after) {
        AfterCallbackScope scope;
        g_after(domain::frameTickSeconds(tickerDifference));
    }
    --g_depth;
}

bool copy(std::uintptr_t address, void* output, std::size_t size) {
    SIZE_T copied = 0;
    return address && ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address),
        output, size, &copied) && copied == size;
}

bool executable(std::uintptr_t address) {
    MEMORY_BASIC_INFORMATION info{};
    if (!address || !VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) ||
        info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return false;
    const auto p = info.Protect & 0xff;
    return p == PAGE_EXECUTE || p == PAGE_EXECUTE_READ || p == PAGE_EXECUTE_READWRITE || p == PAGE_EXECUTE_WRITECOPY;
}

std::wstring moduleBaseName(std::uintptr_t address) {
    MEMORY_BASIC_INFORMATION info{};
    if (!address || !VirtualQuery(reinterpret_cast<void*>(address), &info, sizeof(info)) ||
        !info.AllocationBase) return {};

    wchar_t path[1024]{};
    constexpr DWORD capacity = static_cast<DWORD>(sizeof(path) / sizeof(path[0]));
    const DWORD length = GetModuleFileNameW(
        reinterpret_cast<HMODULE>(info.AllocationBase),
        path,
        capacity
    );
    if (length == 0 || length >= capacity) return {};

    std::wstring result(path, path + length);
    const auto slash = result.find_last_of(L"\\/");
    if (slash != std::wstring::npos) result.erase(0, slash + 1);
    for (auto& ch : result) ch = static_cast<wchar_t>(std::towlower(ch));
    return result;
}

std::string narrowAscii(const std::wstring& text) {
    std::string out;
    out.reserve(text.size());
    for (const wchar_t ch : text) {
        out.push_back(ch >= 0 && ch <= 0x7f ? static_cast<char>(ch) : '?');
    }
    return out;
}

bool knownWidescreenFixWrapper(std::uintptr_t target, const std::wstring& owner) {
    return executable(target) && owner == L"nfsmostwanted.widescreenfix.asi"; // replaced below
}

const char* routeName(domain::GameplayLoopRoute route) {
    switch (route) {
        case domain::GameplayLoopRoute::DirectPinnedTarget: return "direct_pinned_target";
        case domain::GameplayLoopRoute::KnownChainedWrapper: return "known_widescreenfix_chain";
        default: return "blocked";
    }
}

bool attach(std::uintptr_t target, GameplayLoopHook::Callback before, GameplayLoopHook::Callback after) {
    if (g_installed.load() || !executable(target) || (!before && !after)) return false;
    const auto initialized = MH_Initialize();
    if (initialized != MH_OK && initialized != MH_ERROR_ALREADY_INITIALIZED) return false;
    void* original = nullptr;
    const auto created = MH_CreateHook(reinterpret_cast<void*>(target), reinterpret_cast<void*>(&detour), &original);
    if (created != MH_OK) {
        Log::instance().warn(std::string("Gameplay-loop hook creation failed: ") + MH_StatusToString(created));
        return false;
    }
    g_before = before; g_after = after;
    g_original.store(reinterpret_cast<LoopFn>(original));
    g_target.store(target);
    const auto enabled = MH_EnableHook(reinterpret_cast<void*>(target));
    if (enabled != MH_OK) {
        MH_RemoveHook(reinterpret_cast<void*>(target));
        g_original.store(nullptr); g_target.store(0);
    }
    g_installed.store(enabled == MH_OK);
    Log::instance().info(std::string("Gameplay-loop hook installation: ") + MH_StatusToString(enabled));
    return g_installed.load();
}

domain::GameplayLoopResolution discover() {
    const auto base = reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr));
    IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS32 nt{};
    if (!copy(base, &dos, sizeof(dos)) || dos.e_magic != IMAGE_DOS_SIGNATURE ||
        dos.e_lfanew <= 0 || dos.e_lfanew > 1024 * 1024 ||
        !copy(base + dos.e_lfanew, &nt, sizeof(nt)) || nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        nt.OptionalHeader.SizeOfImage > 32 * 1024 * 1024 || nt.OptionalHeader.SizeOfImage < sizeof(nt) ||
        nt.FileHeader.NumberOfSections > 96) return {};
    const auto sections = base + dos.e_lfanew + offsetof(IMAGE_NT_HEADERS32, OptionalHeader) + nt.FileHeader.SizeOfOptionalHeader;
    domain::GameplayLoopResolution combined{};
    for (unsigned i = 0; i < nt.FileHeader.NumberOfSections; ++i) {
        IMAGE_SECTION_HEADER section{};
        if (!copy(sections + i * sizeof(section), &section, sizeof(section))) return {};
        if (!(section.Characteristics & IMAGE_SCN_MEM_EXECUTE)) continue;
        const auto size = section.Misc.VirtualSize;
        if (section.VirtualAddress > nt.OptionalHeader.SizeOfImage || size > nt.OptionalHeader.SizeOfImage - section.VirtualAddress) return {};
        std::vector<std::uint8_t> code(size);
        if (!copy(base + section.VirtualAddress, code.data(), size)) return {};
        const auto found = domain::resolveGameplayLoop(code, base + section.VirtualAddress, base, nt.OptionalHeader.SizeOfImage);
        if (found.match == domain::GameplayLoopMatch::Ambiguous ||
            (found.match != domain::GameplayLoopMatch::Missing && combined.match != domain::GameplayLoopMatch::Missing))
            return {domain::GameplayLoopMatch::Ambiguous, 0, 0};
        if (found.match != domain::GameplayLoopMatch::Missing) combined = found;
    }
    return combined;
}
} // namespace

bool GameplayLoopHook::install(Callback before, Callback after) {
    const auto found = discover();
    std::ostringstream line;
    line << "Gameplay-loop discovery: match=" << static_cast<int>(found.match)
         << " callSite=0x" << std::hex << found.callSite << " target=0x" << found.target
         << " expectedRva=0x263d30 source=MW05_cdecl_int32_fixed_ms callbacks=seconds";
    Log::instance().info(line.str());

    const auto owner = moduleBaseName(found.target);
    const bool knownWrapper = knownWidescreenFixWrapper(found.target, owner);
    const auto route = domain::classifyGameplayLoopRoute(found.match, knownWrapper);

    {
        std::ostringstream routeLine;
        routeLine << "Gameplay-loop route: targetOwner="
                  << (owner.empty() ? "unresolved" : narrowAscii(owner))
                  << " authorization=" << routeName(route);
        Log::instance().info(routeLine.str());
    }

    if (route == domain::GameplayLoopRoute::Blocked) {
        Log::instance().warn(
            "Missing, ambiguous or unsupported redirected main-loop CALL; no gameplay hook or unverified ABI fallback installed."
        );
        return false;
    }

    if (route == domain::GameplayLoopRoute::KnownChainedWrapper) {
        Log::instance().info(
            "Main-loop CALL is already chained through the recognized NFSMostWanted.WidescreenFix.asi wrapper; observing that wrapper entry preserves the existing chain."
        );
    }

    g_site.store(found.callSite);
    const bool installed = attach(found.target, before, after);
    g_verified.store(installed);
    return installed;
}

bool GameplayLoopHook::isInAfterCallback() {
    return g_inAfterCallback && g_depth == 1;
}

GameplayLoopSnapshot GameplayLoopHook::snapshot() {
    return {g_installed.load(), g_verified.load(), g_consistent.load(), g_site.load(), g_target.load(),
        g_entered.load(), g_completed.load(), g_thread.load()};
}

#ifdef FRR_GAMEPLAY_HOOK_TESTING
bool GameplayLoopHook::attachForTest(void* target, Callback before, Callback after) {
    return attach(reinterpret_cast<std::uintptr_t>(target), before, after);
}
void GameplayLoopHook::detachForTest() {
    if (g_installed.load()) {
        MH_DisableHook(reinterpret_cast<void*>(g_target.load()));
        MH_RemoveHook(reinterpret_cast<void*>(g_target.load()));
    }
    g_installed.store(false); g_verified.store(false); g_target.store(0); g_original.store(nullptr);
}
#endif
} // namespace frr::game
