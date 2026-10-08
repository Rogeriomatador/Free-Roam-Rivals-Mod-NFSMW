#include "NativeVehicleFactory.h"
#include "GameBridge.h"
#include "GameplayLoopHook.h"
#include "../core/VersionGuard.h"
#include "../domain/NativeFactorySafety.h"
#include <windows.h>
#include <MinHook.h>
#include <mwsdk/game/mw05.hpp>
#include <NFSPluginSDK/Game.MW05/MW05.h>
#include <cmath>
#include <cstring>

namespace frr::game {
namespace {
using namespace NFSPluginSDK::MW05;
constexpr unsigned kMaxRegistry = 512;
struct Registry {
    unsigned liveCount = 0, physicalCount = 0;
    std::uintptr_t storage = 0;
    std::uintptr_t live[kMaxRegistry]{}, physical[kMaxRegistry]{};
};
struct Context {
    std::uintptr_t player = 0, road = 0, race = 0;
    std::uint64_t profile = 0;
};
struct Owned {
    Context context{};
    std::uintptr_t p = 0, iv = 0, sim = 0;
    std::uint32_t handle = 0, key = 0;
    bool removing = false;
    unsigned absentSamples = 0;
    std::uint64_t lastAbsenceFrame = 0;
};
Owned owned;
bool disabled = false, prepared = false;
// Only the verified gameplay thread may access adapter state. During the
// synchronous native constructor, suppress the capacity routine's eviction
// path. Outside that narrow scope the original behavior remains in place.
volatile LONG constructionThread = 0;
using CapacityFn = bool(__cdecl*)(const void*, bool);
CapacityFn originalCapacity = nullptr;

bool copy(std::uintptr_t address, void* out, std::size_t size) {
    SIZE_T got = 0;
    return address && ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address),
        out, size, &got) && got == size;
}
std::uint64_t fingerprint(std::uintptr_t address) {
    unsigned char bytes[256]{};
    if (!copy(address, bytes, sizeof(bytes))) return 0;
    std::uint64_t hash = 14695981039346656037ull;
    for (auto byte : bytes) hash = (hash ^ byte) * 1099511628211ull;
    return hash;
}
bool gameplayThread() {
    const auto loop = GameplayLoopHook::snapshot();
    return loop.installed && loop.sourceVerified && loop.threadConsistent &&
        loop.completed && loop.threadId == GetCurrentThreadId();
}
bool __cdecl capacityDetour(const void* position, bool extended) {
    if (static_cast<DWORD>(InterlockedCompareExchange(&constructionThread, 0, 0)) == GetCurrentThreadId()) {
        std::uint32_t count = 0;
        const bool readable = copy(0x9377C8, &count, sizeof(count));
        return domain::nativeFactoryCapacitySafe(readable, count, extended);
    }
    return originalCapacity && originalCapacity(position, extended);
}
bool prepareCalls() {
    if (prepared) return true;
    if (!VersionGuard::checkCurrentExecutable().supported) return false;
    struct Expected { std::uintptr_t address; std::uint64_t hash; };
    constexpr Expected functions[] = {
        {0x689820, 0xc77ba20d474f64eeull}, {0x4040F0, 0xe6c69bbf5b67a03dull},
        {0x6876E0, 0xdf69b217075599b6ull}, {0x422480, 0x849130427be78946ull},
        {0x6851D0, 0xef8e39a56391f160ull}, {0x6ED260, 0xcd8425b463193ce0ull}
    };
    for (const auto& f : functions) if (fingerprint(f.address) != f.hash) return false;
    const auto init = MH_Initialize();
    if (init != MH_OK && init != MH_ERROR_ALREADY_INITIALIZED) return false;
    void* original = nullptr;
    if (MH_CreateHook(reinterpret_cast<void*>(0x6ED260), reinterpret_cast<void*>(&capacityDetour), &original) != MH_OK)
        return false;
    originalCapacity = reinterpret_cast<CapacityFn>(original);
    if (MH_EnableHook(reinterpret_cast<void*>(0x6ED260)) != MH_OK) {
        MH_RemoveHook(reinterpret_cast<void*>(0x6ED260));
        originalCapacity = nullptr;
        return false;
    }
    prepared = true;
    return true;
}
// POD-only SEH boundaries: no SDK object allocation or destructors.
bool registry(Registry& out) {
#if defined(_MSC_VER)
    __try {
#endif
        out = Registry{};
        out.liveCount = mwsdk::mw05::vehicle_count();
        out.storage = reinterpret_cast<std::uintptr_t>(mwsdk::runtime::read_absolute<void*>(
            mwsdk::mw05::process(), mwsdk::mw05::db::data::PVehicle_mVehicleListData));
        if (out.liveCount >= kMaxRegistry || (out.liveCount && !out.storage)) return false;
        for (unsigned i = 0; i < out.liveCount; ++i) {
            out.live[i] = reinterpret_cast<std::uintptr_t>(mwsdk::mw05::vehicle_at(i));
            if (!out.live[i]) return false;
        }
        for (; out.physicalCount < kMaxRegistry; ++out.physicalCount) {
            const auto ptr = reinterpret_cast<std::uintptr_t>(PVehicle::g_mInstances[out.physicalCount].mInstance);
            if (!ptr) break;
            out.physical[out.physicalCount] = ptr;
        }
        if (out.physicalCount == kMaxRegistry) return false;
        if (mwsdk::mw05::vehicle_count() != out.liveCount ||
            reinterpret_cast<std::uintptr_t>(mwsdk::runtime::read_absolute<void*>(mwsdk::mw05::process(),
                mwsdk::mw05::db::data::PVehicle_mVehicleListData)) != out.storage) return false;
        for (unsigned i = 0; i < out.liveCount; ++i)
            if (out.live[i] != reinterpret_cast<std::uintptr_t>(mwsdk::mw05::vehicle_at(i))) return false;
        for (unsigned i = 0; i < out.physicalCount; ++i)
            if (out.physical[i] != reinterpret_cast<std::uintptr_t>(PVehicle::g_mInstances[i].mInstance)) return false;
        return !PVehicle::g_mInstances[out.physicalCount].mInstance;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
bool contains(const std::uintptr_t* list, unsigned count, std::uintptr_t ptr) {
    for (unsigned i = 0; i < count; ++i) if (list[i] == ptr) return true;
    return false;
}
bool preserved(const Registry& before, const Registry& after) {
    for (unsigned i = 0; i < before.liveCount; ++i)
        if (!contains(after.live, after.liveCount, before.live[i])) return false;
    for (unsigned i = 0; i < before.physicalCount; ++i)
        if (!contains(after.physical, after.physicalCount, before.physical[i])) return false;
    return true;
}
bool same(const Context& a, const Context& b) {
    return a.player == b.player && a.road == b.road && a.race == b.race && a.profile == b.profile;
}
bool freshWorld(Context& out) {
    if (!gameplayThread()) return false;
    const auto current = GameBridge::sample();
    if (current.mode != WorldProbeMode::FreeRoamCandidate || current.fadeScreen || current.inNIS ||
        current.raceStatusLoading || !current.vehicles.independentPlayerCrossCheck ||
        !current.career.profileKeyAvailable || !current.roadNetwork || !current.raceStatus) return false;
    out = {current.vehicles.playerIVehicle, current.roadNetwork, current.raceStatus, current.career.profileKey};
    return true;
}
bool pursuitClear(const Context& context, const Registry& list) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!contains(list.live, list.liveCount, context.player)) return false;
        auto* player = reinterpret_cast<IVehicle*>(context.player);
        std::uintptr_t vt = 0;
        if (!copy(context.player, &vt, sizeof(vt)) || vt != 0x8AA828) return false;
        auto* ai = player->GetAIVehiclePtr();
        if (!ai) return false;
        std::uintptr_t aiVt = 0, getter = 0;
        if (!copy(reinterpret_cast<std::uintptr_t>(ai), &aiVt, sizeof(aiVt)) ||
            !copy(aiVt + 40 * sizeof(void*), &getter, sizeof(getter)) || getter != 0x431D70) return false;
        // Target getter is MOV EAX,[ECX+70h]; RET. Refuse modified code.
        unsigned char code[4]{};
        if (!copy(getter, code, sizeof(code)) || code[0] != 0x8B || code[1] != 0x41 ||
            code[2] != 0x70 || code[3] != 0xC3) return false;
        std::uintptr_t pursuit = 0;
        if (!copy(reinterpret_cast<std::uintptr_t>(ai) + 0x70, &pursuit, sizeof(pursuit)) || pursuit) return false;
        auto* race = GRaceStatus::Get();
        return reinterpret_cast<std::uintptr_t>(race) == context.race && race &&
            !race->mIsLoading && race->mPlayMode == GRaceStatus::PlayMode::Roaming &&
            !race->mPlayerPursuitInCooldown;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
bool identity(const Registry& list, const Owned& token, bool checkHandle) {
#if defined(_MSC_VER)
    __try {
#endif
        // Membership BEFORE any read through a remembered pointer.
        if (!contains(list.physical, list.physicalCount, token.p) ||
            !contains(list.live, list.liveCount, token.iv)) return false;
        auto* vehicle = reinterpret_cast<IVehicle*>(token.iv);
        std::uintptr_t vt = 0;
        if (!copy(token.iv, &vt, sizeof(vt)) || vt != 0x8AA828) return false;
        auto* sim = vehicle->GetSimable();
        return reinterpret_cast<std::uintptr_t>(sim) == token.sim && sim &&
            (!checkHandle || sim->_mHandle == token.handle) &&
            !sim->IsPlayer() && !sim->IsOwnedByPlayer() && vehicle->GetVehicleKey() == token.key;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
bool constructCall(const NativeFactoryRequest& request, Owned& out) {
#if defined(_MSC_VER)
    __try {
#endif
        UMath::Vector3 position{}, forward{};
        // SDK labels are y,z,x in memory; write the target's x,y,z ABI.
        position.y = request.position.x; position.z = request.position.y; position.x = request.position.z;
        forward.y = request.forward.x; forward.z = request.forward.y; forward.x = request.forward.z;
        // No borrowed customization/performance/cache pointers. A null cache
        // skips 0x689B89's virtual GetCacheName call; capacity eviction is gated
        // separately. Stock attributes decide the appearance of this seed.
        VehicleParams params(DriverClass::Traffic, request.vehicleKey, forward, position,
            nullptr, eVehicleParamFlags::SnapToGround | eVehicleParamFlags::CalcPerformance,
            nullptr, nullptr);
        auto* p = PVehicle::Construct(params);
        if (!p) return false;
        out.p = reinterpret_cast<std::uintptr_t>(p);
        out.iv = reinterpret_cast<std::uintptr_t>(static_cast<IVehicle*>(p));
        out.sim = reinterpret_cast<std::uintptr_t>(static_cast<ISimable*>(p));
        out.key = request.vehicleKey;
        return true;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
bool deactivateAndStamp(Owned& token) {
#if defined(_MSC_VER)
    __try {
#endif
        auto* v = reinterpret_cast<IVehicle*>(token.iv);
        v->Deactivate();
        token.handle = reinterpret_cast<ISimable*>(token.sim)->_mHandle;
        return !v->IsActive();
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
bool racerCall() {
#if defined(_MSC_VER)
    __try {
#endif
        auto* v = reinterpret_cast<IVehicle*>(owned.iv);
        if (v->IsActive() || v->IsLoading() || v->IsDestroyed()) return false;
        v->SetDriverClass(DriverClass::Racer);
        // Driver conversion replaces the AI. Resolve it again, never reuse the
        // traffic AI pointer. The pinned SDK cast supplies the base adjustment.
        auto* ai = v->GetAIVehiclePtr();
        if (v->IsActive() || !ai || *reinterpret_cast<std::uintptr_t*>(ai) != 0x892640) return false;
        auto* primary = static_cast<AIVehicle*>(ai);
        const UCrc32 racerGoal(0x08D0D8A7);
        // Do not call SDK SetGoal: it additionally destroys the old goal in an
        // inline helper. The verified native function handles replacement itself.
        reinterpret_cast<void(__thiscall*)(AIVehicle*, const UCrc32&)>(0x422480)(primary, racerGoal);
        auto* goal = primary->GetGoal();
        return goal && *reinterpret_cast<std::uintptr_t*>(goal) == 0x892D30 && !v->IsActive();
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
bool removalCall() {
#if defined(_MSC_VER)
    __try {
#endif
        auto* v = reinterpret_cast<IVehicle*>(owned.iv);
        if (v->IsLoading() || v->IsDestroyed()) return false;
        v->Deactivate();
        // Kill schedules native retirement; never delete/free a game object.
        reinterpret_cast<void(__thiscall*)(ISimable*)>(0x6851D0)(reinterpret_cast<ISimable*>(owned.sim));
        return true;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
NativeFactoryResult fault() { disabled = true; return NativeFactoryResult::Faulted; }
}

NativeFactoryResult NativeVehicleFactory::constructInactive(const NativeFactoryRequest& request) {
    if (!gameplayThread()) return NativeFactoryResult::Blocked;
    if (disabled || owned.p || !domain::evaluateSpawnCandidate(request.environment, request.candidate).allowed ||
        !request.vehicleKey) return NativeFactoryResult::Blocked;
    const auto& p = request.position;
    const auto& f = request.forward;
    const float length = f.x*f.x + f.y*f.y + f.z*f.z;
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
        !std::isfinite(length) || length < 0.99f || length > 1.01f) return NativeFactoryResult::Blocked;
    Context context{};
    Registry before{};
    if (!freshWorld(context) || !registry(before) || !pursuitClear(context, before) || !prepareCalls())
        return NativeFactoryResult::Blocked;
    Owned created{};
    created.context = context;
    InterlockedExchange(&constructionThread, static_cast<LONG>(GetCurrentThreadId()));
    const bool constructed = constructCall(request, created);
    InterlockedExchange(&constructionThread, 0);
    // A failed/partial native allocation cannot be safely guessed or retried.
    if (!constructed) return fault();
    Registry after{};
    if (contains(before.physical, before.physicalCount, created.p) ||
        contains(before.live, before.liveCount, created.iv) || !registry(after) || !preserved(before, after) ||
        !identity(after, created, false)) return fault();
    owned = created;
    if (!deactivateAndStamp(owned)) return fault();
    return NativeFactoryResult::ConstructedInactive;
}
NativeFactoryResult NativeVehicleFactory::prepareRacerInactive() {
    if (!gameplayThread()) return NativeFactoryResult::Blocked;
    if (disabled || !owned.p || owned.removing) return NativeFactoryResult::Blocked;
    Context context{}; Registry list{};
    if (!freshWorld(context) || !same(context, owned.context) || !registry(list) || !pursuitClear(context, list))
        return NativeFactoryResult::Blocked;
    if (!identity(list, owned, true) || !racerCall()) return fault();
    return NativeFactoryResult::RacerPreparedInactive;
}
NativeFactoryResult NativeVehicleFactory::requestRemoval() {
    if (!gameplayThread()) return NativeFactoryResult::Blocked;
    // Cleanup may still be attempted after an AI preparation fault, but only
    // with the same fresh native identity and a clear pursuit/world context.
    if (!owned.p || owned.removing) return NativeFactoryResult::Blocked;
    Context context{}; Registry list{};
    if (!freshWorld(context) || !same(context, owned.context) || !registry(list) || !pursuitClear(context, list))
        return NativeFactoryResult::Blocked;
    if (!identity(list, owned, true) || !removalCall()) return fault();
    owned.removing = true;
    return NativeFactoryResult::RemovalRequested;
}
NativeFactoryResult NativeVehicleFactory::observeRemoval() {
    if (!gameplayThread() || !owned.removing) return NativeFactoryResult::Blocked;
    Context context{}; Registry list{};
    if (!freshWorld(context) || !same(context, owned.context) || !registry(list)) return NativeFactoryResult::Blocked;
    // Numerical membership only. Kill may have made all remembered pointers
    // invalid; no method or field of the retired object is read here.
    if (contains(list.live, list.liveCount, owned.iv) || contains(list.physical, list.physicalCount, owned.p)) {
        owned.absentSamples = 0;
        return NativeFactoryResult::RemovalPending;
    }
    const auto frame = GameplayLoopHook::snapshot().completed;
    if (owned.lastAbsenceFrame != frame) {
        owned.lastAbsenceFrame = frame;
        ++owned.absentSamples;
    }
    if (owned.absentSamples < 2) return NativeFactoryResult::RemovalPending;
    owned = Owned{};
    return NativeFactoryResult::Removed;
}
}
