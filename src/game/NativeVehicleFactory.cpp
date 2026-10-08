#include "NativeVehicleFactory.h"
#include "GameBridge.h"
#include "GameplayLoopHook.h"
#include "../core/VersionGuard.h"
#include "../core/Log.h"
#include <sstream>
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
    NativeRoadTarget roadTarget{};
    std::uintptr_t p = 0, iv = 0, sim = 0;
    std::uint32_t handle = 0, key = 0;
    bool removing = false, racerPrepared = false, roadPrepared = false;
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
    return GameplayLoopHook::isInAfterCallback() && loop.installed && loop.sourceVerified && loop.threadConsistent &&
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
        {0x6851D0, 0xef8e39a56391f160ull}, {0x6ED260, 0xcd8425b463193ce0ull},
        {0x422690, 0x5fd6cdf5815081cdull}, {0x777660, 0x6e33c2f881b6cb6full},
        {0x415D00, 0x0f74f27ee4f83c3full}, {0x6693A0, 0xe9546f91bb8f8271ull},
        {0x6693C0, 0x8712e1436a826881ull}
    };
    for (const auto& f : functions) {
        const auto actual=fingerprint(f.address);
        if (actual!=f.hash) {
            std::ostringstream line;
            line<<"NativeFactory compatibility blocked address=0x"<<std::hex<<f.address
                <<" expected=0x"<<f.hash<<" actual=0x"<<actual;
            Log::instance().warn(line.str());
            return false;
        }
    }
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
        out.storage = reinterpret_cast<std::uintptr_t>(mwsdk::mw05::read<void**>(
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
            reinterpret_cast<std::uintptr_t>(mwsdk::mw05::read<void**>(mwsdk::mw05::process(),
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
    if (reinterpret_cast<std::uintptr_t>(static_cast<IVehicle*>(reinterpret_cast<PVehicle*>(current.vehicles.playerPVehicle))) !=
        current.vehicles.playerIVehicle) return false;
    out = {current.vehicles.playerIVehicle, current.roadNetwork, current.raceStatus, current.career.profileKey};
    return true;
}
domain::PursuitSafetyState readPursuit(const Context& context, const Registry& list) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!contains(list.live, list.liveCount, context.player)) return domain::PursuitSafetyState::Unknown;
        auto* player = reinterpret_cast<IVehicle*>(context.player);
        std::uintptr_t vt = 0;
        if (!copy(context.player, &vt, sizeof(vt)) || vt != 0x8AA828) return domain::PursuitSafetyState::Unknown;
        auto* ai = player->GetAIVehiclePtr();
        if (!ai) return domain::PursuitSafetyState::Unknown;
        std::uintptr_t aiVt = 0, getter = 0;
        if (!copy(reinterpret_cast<std::uintptr_t>(ai), &aiVt, sizeof(aiVt)) ||
            !copy(aiVt + 40 * sizeof(void*), &getter, sizeof(getter)) || getter != 0x431D70) return domain::PursuitSafetyState::Unknown;
        // Target getter is MOV EAX,[ECX+70h]; RET. Refuse modified code.
        unsigned char code[4]{};
        if (!copy(getter, code, sizeof(code)) || code[0] != 0x8B || code[1] != 0x41 ||
            code[2] != 0x70 || code[3] != 0xC3) return domain::PursuitSafetyState::Unknown;
        std::uintptr_t pursuit = 0;
        if (!copy(reinterpret_cast<std::uintptr_t>(ai) + 0x70, &pursuit, sizeof(pursuit))) return domain::PursuitSafetyState::Unknown;
        if (pursuit) return domain::PursuitSafetyState::Active;
        auto* race = GRaceStatus::Get();
        if (!race || reinterpret_cast<std::uintptr_t>(race) != context.race ||
            race->mIsLoading || race->mPlayMode != GRaceStatus::PlayMode::Roaming)
            return domain::PursuitSafetyState::Unknown;
        return race->mPlayerPursuitInCooldown ? domain::PursuitSafetyState::Cooldown : domain::PursuitSafetyState::Clear;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return domain::PursuitSafetyState::Unknown; }
#endif
}
bool pursuitClear(const Context& context, const Registry& list) {
    return readPursuit(context, list) == domain::PursuitSafetyState::Clear;
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

// Capture scalar navigation only. SDK GetCurrentRoad/GetFutureRoad call
// UpdateRoads, so no navigation virtual getter is called on an anchor vehicle.
bool readRoadTarget(std::uintptr_t identity, bool future, NativeRoadTarget& out) {
#if defined(_MSC_VER)
    __try {
#endif
        auto* v = reinterpret_cast<IVehicle*>(identity);
        std::uintptr_t vt = 0;
        if (!copy(identity, &vt, sizeof(vt)) || vt != 0x8AA828 || !v->IsActive() || v->IsLoading() || v->IsDestroyed()) return false;
        const auto driver = v->GetDriverClass();
        if (driver != DriverClass::Traffic && driver != DriverClass::Racer) return false;
        auto* ai = v->GetAIVehiclePtr();
        if (!ai) return false;
        const auto table = *reinterpret_cast<const std::uintptr_t* const*>(ai);
        if (!table || table[45] != 0x442A70 || table[46] != 0x442A90) return false;
        auto* primary = static_cast<AIVehicle*>(ai);
        const auto* nav = future ? reinterpret_cast<const WRoadNav*>(reinterpret_cast<std::uintptr_t>(ai)+0x3DC) : &primary->mCurrentRoad;
        std::int8_t lane = -1, deadEnd = 0; float laneOffset = 0;
        const auto address = reinterpret_cast<std::uintptr_t>(nav);
        if (!copy(address+0x2C0, &deadEnd, sizeof(deadEnd)) || !copy(address+0x2C1, &lane, sizeof(lane)) ||
            !copy(address+0x2C4, &laneOffset, sizeof(laneOffset))) return false;
        domain::NativeRoadSeed first{nav->fSegmentInd, nav->fNodeInd, lane,
            nav->fSegTime, laneOffset, nav->fValid};
        const auto pos = canonicalMwVector(nav->fPosition);
        const auto forward = canonicalMwVector(nav->fForwardVector);
        if (!copy(address+0x2C1, &lane, sizeof(lane)) || !copy(address+0x2C4, &laneOffset, sizeof(laneOffset))) return false;
        const domain::NativeRoadSeed last{nav->fSegmentInd, nav->fNodeInd, lane,
            nav->fSegTime, laneOffset, nav->fValid};
        if (!domain::validNativeRoadSeed(first) || !domain::validNativeRoadSeed(last) ||
            first.segment != last.segment || first.node != last.node || first.lane != last.lane ||
            first.segmentTime != last.segmentTime || first.laneOffset != last.laneOffset || deadEnd ||
            !std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) return false;
        const float len = std::sqrt(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
        if (!std::isfinite(len) || len < 0.95f || len > 1.05f) return false;
        out.seed = first; out.position = pos;
        out.forward = {forward.x/len, forward.y/len, forward.z/len};
        return true;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
bool targetContext(const NativeRoadTarget& target, const Context& context) {
    std::uintptr_t segments = 0;
    unsigned char segment[22]{};
    const auto now = GetTickCount64();
    return domain::validNativeRoadSeed(target.seed) && target.player == context.player &&
        target.road == context.road && target.race == context.race && target.profile == context.profile &&
        target.millis && now >= target.millis && now - target.millis <= 5000 &&
        copy(0x9B38C0, &segments, sizeof(segments)) && segments && segments == target.segmentTable &&
        copy(segments + 22u * static_cast<unsigned>(target.seed.segment), segment, sizeof(segment));
}
bool ownedRead(NativeOwnedSnapshot& out) {
#if defined(_MSC_VER)
    __try {
#endif
        auto* v = reinterpret_cast<IVehicle*>(owned.iv);
        out.loading = v->IsLoading(); out.active = v->IsActive(); out.destroyed = v->IsDestroyed();
        out.racerPrepared = owned.racerPrepared; out.roadPrepared = owned.roadPrepared;
        if (out.loading || out.destroyed) { out.available = true; return true; }
        auto* sim = v->GetSimable();
        auto* body = sim ? sim->GetRigidBody() : nullptr;
        if (!body) return false;
        UMath::Vector3 right{}, up{}, forward{}, dimensions{};
        body->GetRightVector(right); body->GetUpVector(up); body->GetForwardVector(forward); body->GetDimension(dimensions);
        const auto position = canonicalMwVector(body->GetPosition());
        const auto r = canonicalMwVector(right), u = canonicalMwVector(up), f = canonicalMwVector(forward), d = canonicalMwVector(dimensions);
        out.box = domain::makeVehicleOrientedBox(owned.iv, owned.key,
            {position.x,position.y,position.z}, {r.x,r.y,r.z}, {u.x,u.y,u.z}, {f.x,f.y,f.z}, {d.x,d.y,d.z});
        out.speed = v->GetAbsoluteSpeed();
        out.available = out.box.valid && std::isfinite(out.speed);
        return out.available;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
bool ownedPursuitClear() {
#if defined(_MSC_VER)
    __try {
#endif
        auto* ai = reinterpret_cast<IVehicle*>(owned.iv)->GetAIVehiclePtr();
        if (!ai) return false;
        const auto table = *reinterpret_cast<const std::uintptr_t* const*>(ai);
        std::uintptr_t pursuit = 0;
        return table && table[40] == 0x431D70 && copy(reinterpret_cast<std::uintptr_t>(ai) + 0x70, &pursuit, sizeof(pursuit)) && !pursuit;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
bool roadResetCall() {
#if defined(_MSC_VER)
    __try {
#endif
        auto* v = reinterpret_cast<IVehicle*>(owned.iv);
        if (v->IsActive() || v->IsLoading() || v->IsDestroyed()) return false;
        auto* ai = v->GetAIVehiclePtr();
        if (!ai || *reinterpret_cast<std::uintptr_t*>(ai) != 0x892640) return false;
        domain::NativeRoadSeedBytes source{};
        if (!domain::encodeNativeRoadSeed(owned.roadTarget.seed, source)) return false;
        // Actual target returns AL success (the SDK declares void).
        const bool reset = reinterpret_cast<bool(__thiscall*)(IVehicleAI*, const void*)>(0x422690)(ai, source.bytes.data());
        if (!reset) return false;
        NativeOwnedSnapshot current{};
        if (!ownedRead(current) || !current.box.valid || current.active) return false;
        const float dx = current.box.center.x - owned.roadTarget.position.x;
        const float dz = current.box.center.z - owned.roadTarget.position.z;
        return dx*dx + dz*dz <= 1.0f;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
bool activationCall() {
#if defined(_MSC_VER)
    __try {
#endif
        auto* v = reinterpret_cast<IVehicle*>(owned.iv);
        if (v->IsLoading() || v->IsDestroyed() || v->IsActive()) return false;
        auto* ai = v->GetAIVehiclePtr();
        if (!ai || *reinterpret_cast<std::uintptr_t*>(ai) != 0x892640) return false;
        ai->SetSpawned();
        // 0x415D00 calls 0x414CE0, which clears the old goal. Assign the
        // Racer goal AFTER SetSpawned, rather than losing it at activation.
        ai = v->GetAIVehiclePtr();
        if (!ai || *reinterpret_cast<std::uintptr_t*>(ai) != 0x892640) return false;
        auto* primary = static_cast<AIVehicle*>(ai);
        const UCrc32 racerGoal(0x08D0D8A7);
        reinterpret_cast<void(__thiscall*)(AIVehicle*, const UCrc32&)>(0x422480)(primary, racerGoal);
        auto* goal = primary->GetGoal();
        if (!goal || *reinterpret_cast<std::uintptr_t*>(goal) != 0x892D30) return false;
        v->Activate();
        return v->IsActive();
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
NativeFactoryResult fault() { disabled = true; return NativeFactoryResult::Faulted; }
}


domain::PursuitSafetyState NativeVehicleFactory::pursuitState() {
    Context context{}; Registry list{};
    if (!freshWorld(context) || !registry(list)) return domain::PursuitSafetyState::Unknown;
    return readPursuit(context, list);
}
std::vector<NativeRoadTarget> NativeVehicleFactory::captureRoadTargets() {
    std::vector<NativeRoadTarget> out;
    Context context{}; Registry before{}, after{};
    std::uintptr_t segments = 0;
    if (!freshWorld(context) || !registry(before) || !pursuitClear(context, before) ||
        !copy(0x9B38C0, &segments, sizeof(segments)) || !segments) return out;
    for (unsigned i = 0; i < before.liveCount && out.size() < 32; ++i) {
        for (bool future : {true,false}) {
            NativeRoadTarget target{};
            if (!readRoadTarget(before.live[i], future, target)) continue;
            target.player = context.player; target.road = context.road; target.race = context.race;
            target.profile = context.profile; target.segmentTable = segments; target.millis = GetTickCount64();
            if (targetContext(target, context)) out.push_back(target);
        }
    }
    if (!registry(after) || before.liveCount != after.liveCount || before.storage != after.storage || !preserved(before, after)) out.clear();
    return out;
}
NativeOwnedSnapshot NativeVehicleFactory::snapshot() {
    NativeOwnedSnapshot out{};
    if (!gameplayThread() || !owned.p) return out;
    out.owned = true;
    Context context{}; Registry list{};
    if (!freshWorld(context) || !same(context, owned.context) || !registry(list)) return out;
    out.contextMatches = true;
    if (!identity(list, owned, true)) return out;
    out.pursuit = readPursuit(context, list);
    ownedRead(out);
    return out;
}
NativeFactoryResult NativeVehicleFactory::resetRoadInactive() {
    if (!gameplayThread() || disabled || !owned.p || !owned.racerPrepared || owned.roadPrepared || owned.removing)
        return NativeFactoryResult::Blocked;
    Context context{}; Registry list{};
    if (!freshWorld(context) || !same(context, owned.context) || !registry(list) || !pursuitClear(context, list) ||
        !targetContext(owned.roadTarget, context)) return NativeFactoryResult::Blocked;
    if (!identity(list, owned, true) || !ownedPursuitClear() || !roadResetCall()) return fault();
    owned.roadPrepared = true;
    return NativeFactoryResult::RoadPreparedInactive;
}
NativeFactoryResult NativeVehicleFactory::activatePrepared(const domain::SpawnEnvironmentInput& environment,
    const domain::SpawnCandidateInput& candidate) {
    if (!gameplayThread() || disabled || !owned.p || !owned.roadPrepared || owned.removing ||
        !domain::evaluateSpawnCandidate(environment, candidate).allowed) return NativeFactoryResult::Blocked;
    Context context{}; Registry list{};
    if (!freshWorld(context) || !same(context, owned.context) || !registry(list) || !pursuitClear(context, list) ||
        !targetContext(owned.roadTarget, context)) return NativeFactoryResult::Blocked;
    if (!identity(list, owned, true) || !ownedPursuitClear() || !activationCall()) return fault();
    return NativeFactoryResult::Activated;
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
    std::uint32_t physicalCount = 0;
    if (!domain::nativeFactoryCapacitySafe(copy(0x9377C8, &physicalCount, sizeof(physicalCount)), physicalCount, true))
        return NativeFactoryResult::Blocked;
    Owned created{};
    if (!targetContext(request.roadTarget, context) ||
        request.position.x != request.roadTarget.position.x || request.position.y != request.roadTarget.position.y ||
        request.position.z != request.roadTarget.position.z) return NativeFactoryResult::Blocked;
    created.context = context;
    created.roadTarget = request.roadTarget;
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
    if (disabled || !owned.p || owned.removing || owned.racerPrepared) return NativeFactoryResult::Blocked;
    Context context{}; Registry list{};
    if (!freshWorld(context) || !same(context, owned.context) || !registry(list) || !pursuitClear(context, list))
        return NativeFactoryResult::Blocked;
    if (!identity(list, owned, true)) return fault();
    if (!ownedPursuitClear()) return NativeFactoryResult::Blocked;
    NativeOwnedSnapshot observation{};
    if (!ownedRead(observation)) return fault();
    if (observation.loading) return NativeFactoryResult::Blocked;
    if (!racerCall()) return fault();
    owned.racerPrepared = true;
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
    if (!identity(list, owned, true)) return fault();
    if (!ownedPursuitClear()) return NativeFactoryResult::Blocked;
    if (!removalCall()) return fault();
    owned.removing = true;
    return NativeFactoryResult::RemovalRequested;
}
NativeFactoryResult NativeVehicleFactory::observeExternalRemoval() {
    if (!gameplayThread() || !owned.p || owned.removing) return NativeFactoryResult::Blocked;
    Context context{}; Registry list{};
    if (!freshWorld(context) || !same(context, owned.context) || !registry(list)) return NativeFactoryResult::Blocked;
    if (contains(list.live,list.liveCount,owned.iv) || contains(list.physical,list.physicalCount,owned.p)) {
        owned.absentSamples=0;
        return NativeFactoryResult::Blocked;
    }
    const auto frame=GameplayLoopHook::snapshot().completed;
    if (owned.lastAbsenceFrame!=frame) { owned.lastAbsenceFrame=frame; ++owned.absentSamples; }
    if (owned.absentSamples<2) return NativeFactoryResult::RemovalPending;
    owned=Owned{}; disabled=true;
    return NativeFactoryResult::RemovedByEngine;
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
