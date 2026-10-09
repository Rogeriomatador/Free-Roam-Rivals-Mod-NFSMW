#include "NativeVehicleFactory.h"
#include "GameBridge.h"
#include "GameplayLoopHook.h"
#include "../core/VersionGuard.h"
#include "../core/Log.h"
#include <sstream>
#include <iomanip>
#include <array>
#include <algorithm>
#include "../domain/NativeFactorySafety.h"
#include "../domain/NativeConstructionConfirmation.h"
#include "../domain/NativeCodeCompatibility.h"
#include "../domain/NativeSearchWindow.h"
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
Owned owned, pending;
Registry pendingBaseline{};
std::uint64_t pendingSince=0, pendingLastFrame=0;
unsigned pendingConfirmations=0;
DWORD lastNativeException=0;
std::uintptr_t lastExceptionInstruction=0,lastExceptionMemory=0;
std::uintptr_t identityVtable=0,identitySim=0;
std::uint32_t identityKey=0;
const char* nativePhase="none";
const char* identityReason="not_checked";
std::size_t sourceBatchCursor = 0;
bool disabled = false, prepared = false, compatibilityRejected = false;
// Only the verified gameplay thread may access adapter state. During the
// synchronous native constructor, suppress the capacity routine's eviction
// path. Outside that narrow scope the original behavior remains in place.
volatile LONG constructionThread = 0;
using CapacityFn = bool(__cdecl*)(const void*, bool);
CapacityFn originalCapacity = nullptr;

int captureNativeException(EXCEPTION_POINTERS* exception) {
    if (exception && exception->ExceptionRecord) {
        const auto* record=exception->ExceptionRecord;
        lastNativeException=record->ExceptionCode;
        lastExceptionInstruction=reinterpret_cast<std::uintptr_t>(record->ExceptionAddress);
        lastExceptionMemory=(record->ExceptionCode==EXCEPTION_ACCESS_VIOLATION && record->NumberParameters>=2)
            ? record->ExceptionInformation[1] : 0;
    }
    return EXCEPTION_EXECUTE_HANDLER;
}
void beginNativeOperation(const char* phase) {
    nativePhase=phase; lastNativeException=0;
    lastExceptionInstruction=0; lastExceptionMemory=0;
}
bool copy(std::uintptr_t address, void* out, std::size_t size) {
    SIZE_T got = 0;
    return address && ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(address),
        out, size, &got) && got == size;
}
// Read the exact MD5-guarded executable from disk, never map/execute it.
struct ExecutableCodeFile {
    HANDLE file=INVALID_HANDLE_VALUE;
    ExecutableCodeFile() {
        wchar_t path[32768]{};
        const auto size=GetModuleFileNameW(nullptr,path,32768);
        if (!size || size>=32768 || reinterpret_cast<std::uintptr_t>(GetModuleHandleW(nullptr))!=0x400000u) return;
        file=CreateFileW(path,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
            nullptr,OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,nullptr);
        if (file==INVALID_HANDLE_VALUE) return;
        LARGE_INTEGER length{};
        if (!GetFileSizeEx(file,&length) || length.QuadPart!=6029312) { CloseHandle(file); file=INVALID_HANDLE_VALUE; }
    }
    ~ExecutableCodeFile() { if (file!=INVALID_HANDLE_VALUE) CloseHandle(file); }
    bool read(std::uint32_t address, unsigned char* bytes, std::size_t size) {
        std::uint32_t offset=0;
        if (file==INVALID_HANDLE_VALUE || !domain::supportedNativeCodeFileOffset(address,size,offset)) return false;
        LARGE_INTEGER position{}; position.QuadPart=offset;
        DWORD got=0;
        return SetFilePointerEx(file,position,nullptr,FILE_BEGIN) &&
            ReadFile(file,bytes,static_cast<DWORD>(size),&got,nullptr) && got==size;
    }
};
std::string codeBytes(const unsigned char* bytes, std::size_t size) {
    std::ostringstream out; out<<std::hex<<std::setfill('0');
    for (std::size_t i=0;i<size;++i) { if (i) out<<' '; out<<std::setw(2)<<unsigned(bytes[i]); }
    return out.str();
}
void logEntryJump(std::uintptr_t address, const unsigned char* bytes) {
    const auto shape=domain::nativeEntryJump(bytes,32);
    if (shape==domain::NativeEntryJump::None) {
        Log::instance().info("NativeFactory entryJump=none; interior patches remain possible"); return;
    }
    std::uint32_t operand=0,target=0; bool readable=true;
    if (shape==domain::NativeEntryJump::RelativeE9) {
        std::memcpy(&operand,bytes+1,4); target=static_cast<std::uint32_t>(address)+5u+operand;
    } else {
        std::memcpy(&operand,bytes+2,4); readable=copy(operand,&target,sizeof(target));
    }
    HMODULE module=nullptr; char path[32768]{};
    if (readable && target && GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCSTR>(static_cast<std::uintptr_t>(target)),&module))
        GetModuleFileNameA(module,path,32768);
    path[32767]='\0';
    const char* name=std::strrchr(path,'\\'); name=name ? name+1 : path;
    std::ostringstream out;
    out<<"NativeFactory entryJump="<<(shape==domain::NativeEntryJump::RelativeE9 ? "E9" : "FF25")
        <<" targetReadable="<<readable<<" target=0x"<<std::hex<<target
        <<" targetModule="<<(*name ? name : "unknown")<<" jumpShapeIsNotHookProof=1";
    Log::instance().warn(out.str());
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
bool auditCallsReadOnly() {
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
    ExecutableCodeFile disk;
    unsigned mismatches=0;
    for (const auto& f : functions) {
        std::array<unsigned char,256> live{},file{};
        const bool liveReadable=copy(f.address,live.data(),live.size());
        const bool fileReadable=disk.read(static_cast<std::uint32_t>(f.address),file.data(),file.size());
        const auto actual=liveReadable ? domain::nativeCodeFingerprint(live.data(),live.size()) : 0;
        const auto fileHash=fileReadable ? domain::nativeCodeFingerprint(file.data(),file.size()) : 0;
        const bool matches=liveReadable && fileReadable && actual==f.hash && fileHash==f.hash;
        std::ostringstream line;
        line<<"NativeFactory compatibility audit address=0x"<<std::hex<<f.address
            <<" expected=0x"<<f.hash<<" actual=0x"<<actual<<" file=0x"<<fileHash
            <<std::dec<<" liveReadable="<<liveReadable<<" fileReadable="<<fileReadable<<" match="<<matches;
        Log::instance().info(line.str());
        if (matches) continue;
        ++mismatches;
        std::ostringstream failure;
        failure<<"NativeFactory compatibility blocked address=0x"<<std::hex<<f.address
            <<" expected=0x"<<f.hash<<" actual=0x"<<actual;
        Log::instance().warn(failure.str());
        if (liveReadable) { Log::instance().warn("NativeFactory liveFirst32="+codeBytes(live.data(),32)); logEntryJump(f.address,live.data()); }
        if (fileReadable) Log::instance().info("NativeFactory fileFirst32="+codeBytes(file.data(),32));
        if (liveReadable && fileReadable) {
            const auto first=domain::firstNativeCodeDifference(live.data(),file.data(),live.size());
            if (first<live.size()) {
                const auto start=std::min(first>8 ? first-8 : std::size_t(0),live.size()-32);
                std::ostringstream difference;
                difference<<"NativeFactory difference firstOffset=0x"<<std::hex<<first<<" firstAddress=0x"<<(f.address+first)
                    <<" windowOffset=0x"<<start<<" liveWindow32="<<codeBytes(live.data()+start,32)
                    <<" fileWindow32="<<codeBytes(file.data()+start,32);
                Log::instance().warn(difference.str());
            }
        }
    }
    std::ostringstream summary;
    summary<<"NativeFactory compatibility audit complete checked=11 mismatches="<<mismatches<<" signaturesMatch="<<(mismatches==0);
    Log::instance().info(summary.str());
    return mismatches==0;
}
bool prepareCalls() {
    if (prepared) return true;
    if (compatibilityRejected) return false;
    if (!auditCallsReadOnly()) { compatibilityRejected=true; return false; }
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
    } __except (captureNativeException(GetExceptionInformation())) { return false; }
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
    identityReason="registry_membership"; identityVtable=0; identitySim=0; identityKey=0;
#if defined(_MSC_VER)
    __try {
#endif
        // No object reads before membership in BOTH fresh registries.
        if (!contains(list.physical, list.physicalCount, token.p) ||
            !contains(list.live, list.liveCount, token.iv)) return false;
        auto* vehicle = reinterpret_cast<IVehicle*>(token.iv);
        std::uintptr_t vt=0;
        identityReason="IVehicle_vtable";
        if (!copy(token.iv,&vt,sizeof(vt))) return false;
        identityVtable=vt;
        if (vt!=0x8AA828) return false;
        identityReason="simable_pointer";
        auto* sim=vehicle->GetSimable();
        identitySim=reinterpret_cast<std::uintptr_t>(sim);
        if (!sim || reinterpret_cast<std::uintptr_t>(sim)!=token.sim) return false;
        identityReason="simable_handle";
        if (checkHandle && sim->_mHandle!=token.handle) return false;
        identityReason="player_or_player_owned";
        if (sim->IsPlayer() || sim->IsOwnedByPlayer()) return false;
        identityReason="vehicle_key";
        identityKey=vehicle->GetVehicleKey();
        if (identityKey!=token.key) return false;
        identityReason="verified";
        return true;
#if defined(_MSC_VER)
    } __except (captureNativeException(GetExceptionInformation())) { identityReason="identity_exception"; return false; }
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
        nativePhase="constructor";
        auto* p = PVehicle::Construct(params);
        nativePhase="constructor_returned";
        if (!p) { nativePhase="constructor_null"; return false; }
        out.p = reinterpret_cast<std::uintptr_t>(p);
        out.iv = reinterpret_cast<std::uintptr_t>(static_cast<IVehicle*>(p));
        out.sim = reinterpret_cast<std::uintptr_t>(static_cast<ISimable*>(p));
        out.key = request.vehicleKey;
        return true;
#if defined(_MSC_VER)
    } __except (captureNativeException(GetExceptionInformation())) { return false; }
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
    } __except (captureNativeException(GetExceptionInformation())) { return false; }
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
    } __except (captureNativeException(GetExceptionInformation())) { return false; }
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
    } __except (captureNativeException(GetExceptionInformation())) { return false; }
#endif
}

// Capture scalar navigation only. SDK GetCurrentRoad/GetFutureRoad call
// UpdateRoads, so no navigation virtual getter is called on an anchor vehicle.
bool readRoadTarget(std::uintptr_t identity, unsigned source, NativeRoadTarget& out, unsigned& rejection) {
    rejection = 1;
#if defined(_MSC_VER)
    __try {
#endif
        auto* v = reinterpret_cast<IVehicle*>(identity);
        std::uintptr_t vt = 0;
        if (!copy(identity, &vt, sizeof(vt)) || vt != 0x8AA828 || !v->IsActive() || v->IsLoading() || v->IsDestroyed()) return false;
        rejection = 2;
        const auto driver = v->GetDriverClass();
        if (driver != DriverClass::Traffic && driver != DriverClass::Racer) return false;
        rejection = 3;
        auto* ai = v->GetAIVehiclePtr();
        if (!ai) return false;
        const auto table = *reinterpret_cast<const std::uintptr_t* const*>(ai);
        if (!table) return false;
        auto* primary = static_cast<AIVehicle*>(ai);
        const WRoadNav* nav = nullptr;
        if (source == 0) {
            // Verified getter 0x431C50 is MOV EAX,[ECX+24h]; RET. Read the
            // existing drive navigation without invoking source AI methods.
            if (table[18] != 0x431C50) return false;
            rejection = 4;
            unsigned char code[4]{};
            std::uintptr_t address = 0;
            if (!copy(0x431C50, code, sizeof(code)) || code[0]!=0x8B || code[1]!=0x41 ||
                code[2]!=0x24 || code[3]!=0xC3 ||
                !copy(reinterpret_cast<std::uintptr_t>(ai)+0x24, &address, sizeof(address)) || !address) return false;
            nav = reinterpret_cast<const WRoadNav*>(address);
        } else {
            if (table[45] != 0x442A70 || table[46] != 0x442A90) return false;
            nav = source == 1 ? reinterpret_cast<const WRoadNav*>(reinterpret_cast<std::uintptr_t>(ai)+0x3DC) : &primary->mCurrentRoad;
        }
        rejection = 5;
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
        rejection = 6;
        if (!domain::validNativeRoadSeed(first) || !domain::validNativeRoadSeed(last) || deadEnd) return false;
        rejection = 7;
        if (source == 0) {
            std::uintptr_t after = 0;
            if (!copy(reinterpret_cast<std::uintptr_t>(ai)+0x24, &after, sizeof(after)) || after != address) return false;
        }
        if (first.segment != last.segment || first.node != last.node || first.lane != last.lane ||
            first.segmentTime != last.segmentTime || first.laneOffset != last.laneOffset) return false;
        rejection = 8;
        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) return false;
        const float len = std::sqrt(forward.x*forward.x + forward.y*forward.y + forward.z*forward.z);
        if (!std::isfinite(len) || len < 0.95f || len > 1.05f) return false;
        out.seed = first; out.position = pos;
        out.forward = {forward.x/len, forward.y/len, forward.z/len};
        rejection = 0;
        return true;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { rejection = 9; return false; }
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
    } __except (captureNativeException(GetExceptionInformation())) { return false; }
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
    } __except (captureNativeException(GetExceptionInformation())) { return false; }
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
    } __except (captureNativeException(GetExceptionInformation())) { return false; }
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
    } __except (captureNativeException(GetExceptionInformation())) { return false; }
#endif
}
NativeFactoryResult fault(const char* reason="native_operation") {
    disabled=true;
    std::ostringstream line;
    line<<"NativeFactory fault reason="<<reason<<" identity="<<identityReason
        <<" phase="<<nativePhase<<" exception=0x"<<std::hex<<lastNativeException
        <<" exceptionInstruction=0x"<<lastExceptionInstruction<<" exceptionMemory=0x"<<lastExceptionMemory
        <<" identityVtable=0x"<<identityVtable<<" identitySim=0x"<<identitySim<<" identityKey=0x"<<identityKey
        <<" expectedKey=0x"<<(owned.p ? owned.key : pending.key)
        <<" ownedP=0x"<<owned.p<<" pendingP=0x"<<pending.p
        <<" retryConstructionAllowed=0";
    Log::instance().warn(line.str());
    return NativeFactoryResult::Faulted;
}
}


bool NativeVehicleFactory::auditCompatibilityReadOnly() {
    if (!gameplayThread()) return false;
    if (prepared || owned.p) {
        Log::instance().warn("NativeFactory read-only audit unavailable: factory already prepared; own capacity hook would invalidate the original entry fingerprint.");
        return false;
    }
    Log::instance().info("NativeFactory read-only audit requested: no capacity hook, constructor, goal, reset, activate or retirement calls");
    return auditCallsReadOnly();
}
domain::PursuitSafetyState NativeVehicleFactory::pursuitState() {
    Context context{}; Registry list{};
    if (!freshWorld(context) || !registry(list)) return domain::PursuitSafetyState::Unknown;
    return readPursuit(context, list);
}
std::vector<NativeRoadTarget> NativeVehicleFactory::captureRoadTargets(std::size_t& batchIndex,
    NativeRoadCaptureReport& report) {
    batchIndex = 0;
    report = {};
    std::vector<NativeRoadTarget> out;
    Context context{}; Registry before{}, after{};
    std::uintptr_t segments = 0;
    report.status = "world_context";
    if (!freshWorld(context)) return out;
    report.status = "registry";
    if (!registry(before)) return out;
    report.liveSlots = before.liveCount;
    report.status = "pursuit";
    if (!pursuitClear(context, before)) return out;
    report.status = "road_segment_table";
    if (!copy(0x9B38C0, &segments, sizeof(segments)) || !segments) return out;
    report.status = "sampled";
    const auto batch = domain::nextNativeSearchWindow((before.liveCount + 15u) / 16u, 1, sourceBatchCursor);
    if (!batch.count) return out;
    batchIndex = batch.index(0);
    const auto start = static_cast<unsigned>(batchIndex * 16u);
    const auto end = std::min(start + 16u, before.liveCount);
    report.sampledSlots = end-start;
    for (unsigned i = start; i < end; ++i) {
        for (unsigned source = 0; source < 3; ++source) {
            NativeRoadTarget target{};
            unsigned rejection = 0;
            if (!readRoadTarget(before.live[i], source, target, rejection)) {
                ++report.rejected[rejection]; continue;
            }
            target.player = context.player; target.road = context.road; target.race = context.race;
            target.profile = context.profile; target.segmentTable = segments; target.millis = GetTickCount64();
            if (targetContext(target, context)) out.push_back(target);
            else ++report.rejectedContext;
        }
    }
    if (!registry(after) || before.liveCount != after.liveCount || before.storage != after.storage || !preserved(before, after)) {
        report.status = "registry_changed"; out.clear();
    }
    report.accepted = static_cast<unsigned>(out.size());
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
    beginNativeOperation("road_preparation");
    if (!gameplayThread() || disabled || !owned.p || !owned.racerPrepared || owned.roadPrepared || owned.removing)
        return NativeFactoryResult::Blocked;
    Context context{}; Registry list{};
    if (!freshWorld(context) || !same(context, owned.context) || !registry(list) || !pursuitClear(context, list) ||
        !targetContext(owned.roadTarget, context)) return NativeFactoryResult::Blocked;
    if (!identity(list,owned,true)) return fault("road_identity_rejected");
    if (!ownedPursuitClear()) return NativeFactoryResult::Blocked;
    nativePhase="road_reset";
    if (!roadResetCall()) return fault("road_reset_rejected");
    owned.roadPrepared = true;
    return NativeFactoryResult::RoadPreparedInactive;
}
NativeFactoryResult NativeVehicleFactory::activatePrepared(const domain::SpawnEnvironmentInput& environment,
    const domain::SpawnCandidateInput& candidate) {
    beginNativeOperation("activation");
    if (!gameplayThread() || disabled || !owned.p || !owned.roadPrepared || owned.removing ||
        !domain::evaluateSpawnCandidate(environment, candidate).allowed) return NativeFactoryResult::Blocked;
    Context context{}; Registry list{};
    if (!freshWorld(context) || !same(context, owned.context) || !registry(list) || !pursuitClear(context, list) ||
        !targetContext(owned.roadTarget, context)) return NativeFactoryResult::Blocked;
    if (!identity(list,owned,true)) return fault("activation_identity_rejected");
    if (!ownedPursuitClear()) return NativeFactoryResult::Blocked;
    nativePhase="SetSpawned_goal_Activate";
    if (!activationCall()) return fault("activation_rejected");
    return NativeFactoryResult::Activated;
}
NativeFactoryResult NativeVehicleFactory::constructInactive(const NativeFactoryRequest& request) {
    beginNativeOperation("construction_preflight");
    if (!gameplayThread()) return NativeFactoryResult::Blocked;
    if (disabled || owned.p || pending.p || !domain::evaluateSpawnCandidate(request.environment, request.candidate).allowed ||
        !request.vehicleKey) return NativeFactoryResult::Blocked;
    const auto& p = request.position;
    const auto& f = request.forward;
    const float length = f.x*f.x + f.y*f.y + f.z*f.z;
    if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.z) ||
        !std::isfinite(length) || length < 0.99f || length > 1.01f) return NativeFactoryResult::Blocked;
    Context context{};
    Registry before{};
    if (!freshWorld(context) || !registry(before) || !pursuitClear(context, before))
        return NativeFactoryResult::Blocked;
    if (!prepareCalls()) return compatibilityRejected ? NativeFactoryResult::CompatibilityBlocked : NativeFactoryResult::Blocked;
    std::uint32_t physicalCount = 0;
    if (!domain::nativeFactoryCapacitySafe(copy(0x9377C8, &physicalCount, sizeof(physicalCount)), physicalCount, true))
        return NativeFactoryResult::Blocked;
    Owned created{};
    if (!targetContext(request.roadTarget, context) ||
        request.position.x != request.roadTarget.position.x || request.position.y != request.roadTarget.position.y ||
        request.position.z != request.roadTarget.position.z) return NativeFactoryResult::Blocked;
    created.context = context;
    created.roadTarget = request.roadTarget;
    Log::instance().info("NativeFactory constructor invoke: compatibility passed; stock GTI inactive staging");
    InterlockedExchange(&constructionThread, static_cast<LONG>(GetCurrentThreadId()));
    lastNativeException=0; nativePhase="vehicle_params";
    const bool constructed = constructCall(request, created);
    InterlockedExchange(&constructionThread, 0);
    // Never retry a constructor: even an exception can leave a partial allocation.
    if (!constructed) return fault("constructor_failed_or_partial");
    if (contains(before.physical,before.physicalCount,created.p) ||
        contains(before.live,before.liveCount,created.iv)) return fault("constructor_returned_existing_identity");
    pending=created;
    pendingBaseline=before;
    pendingSince=GetTickCount64(); pendingLastFrame=0; pendingConfirmations=0;
    std::ostringstream result;
    result<<"NativeFactory constructor returned p=0x"<<std::hex<<created.p
        <<" iv=0x"<<created.iv<<" sim=0x"<<created.sim<<std::dec
        <<" baselineLive="<<before.liveCount<<" baselinePhysical="<<before.physicalCount
        <<" awaitingTwoCompletedFrames=1";
    Log::instance().info(result.str());
    return NativeFactoryResult::ConstructionPending;
}
NativeFactoryResult NativeVehicleFactory::confirmConstructionInactive() {
    beginNativeOperation("construction_confirmation");
    if (!gameplayThread() || disabled || !pending.p) return NativeFactoryResult::Blocked;
    if (GetTickCount64()-pendingSince>2000) return fault("construction_confirmation_timeout");
    Context context{}; Registry list{};
    if (!freshWorld(context) || !same(context,pending.context)) { pendingConfirmations=0; return NativeFactoryResult::ConstructionPending; }
    const bool readable=registry(list);
    if (!readable) { pendingConfirmations=0; return NativeFactoryResult::ConstructionPending; }
    if (!preserved(pendingBaseline,list)) return fault("preexisting_registry_identity_lost");
    const bool membership=contains(list.physical,list.physicalCount,pending.p) &&
        contains(list.live,list.liveCount,pending.iv);
    const bool clear=pursuitClear(context,list);
    const auto frame=GameplayLoopHook::snapshot().completed;
    // completed is a count in the verified hook snapshot, not a render tick.
    const auto decision=domain::advanceNativeConstructionConfirmation(
        frame,readable,membership,clear,pendingLastFrame,pendingConfirmations);
    if (decision!=domain::NativeConstructionDecision::Confirm) {
        std::ostringstream wait;
        wait<<"NativeFactory construction pending live="<<list.liveCount<<" physical="<<list.physicalCount
            <<" bothMembership="<<membership<<" pursuitClear="<<clear<<" confirmations="<<pendingConfirmations;
        Log::instance().info(wait.str());
        return NativeFactoryResult::ConstructionPending;
    }
    if (!identity(list,pending,false)) return fault("construction_identity_rejected");
    owned=pending; pending={}; pendingBaseline={};
    nativePhase="deactivate_and_handle_stamp";
    if (!deactivateAndStamp(owned)) return fault("deactivate_and_handle_stamp_failed");
    Log::instance().info("NativeFactory construction confirmed: identity verified in two completed frames; inactive handle stamped");
    return NativeFactoryResult::ConstructedInactive;
}

NativeFactoryResult NativeVehicleFactory::prepareRacerInactive() {
    beginNativeOperation("Racer_preparation");
    if (!gameplayThread()) return NativeFactoryResult::Blocked;
    if (disabled || !owned.p || owned.removing || owned.racerPrepared) return NativeFactoryResult::Blocked;
    Context context{}; Registry list{};
    if (!freshWorld(context) || !same(context, owned.context) || !registry(list) || !pursuitClear(context, list))
        return NativeFactoryResult::Blocked;
    if (!identity(list, owned, true)) return fault("owned_identity_rejected");
    if (!ownedPursuitClear()) return NativeFactoryResult::Blocked;
    NativeOwnedSnapshot observation{};
    if (!ownedRead(observation)) return fault("owned_body_read_failed");
    if (observation.loading) return NativeFactoryResult::Blocked;
    nativePhase="Racer_driver_and_goal";
    if (!racerCall()) return fault("Racer_driver_or_goal_rejected");
    owned.racerPrepared = true;
    return NativeFactoryResult::RacerPreparedInactive;
}
NativeFactoryResult NativeVehicleFactory::requestRemoval() {
    beginNativeOperation("retirement");
    if (!gameplayThread()) return NativeFactoryResult::Blocked;
    // Cleanup may still be attempted after an AI preparation fault, but only
    // with the same fresh native identity and a clear pursuit/world context.
    if (!owned.p || owned.removing) return NativeFactoryResult::Blocked;
    Context context{}; Registry list{};
    if (!freshWorld(context) || !same(context, owned.context) || !registry(list) || !pursuitClear(context, list))
        return NativeFactoryResult::Blocked;
    if (!identity(list, owned, true)) return fault("owned_identity_rejected");
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
