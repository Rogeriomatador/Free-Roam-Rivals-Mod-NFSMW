#include "PostRaceRacerProbe.h"

#include "GameBridge.h"
#include "GameplayLoopHook.h"
#include "NfsPluginCoordinateAdapter.h"
#include "../core/Log.h"
#include "../domain/PostRaceObservation.h"

#include <windows.h>
#include <mwsdk/game/mw05.hpp>
#include <NFSPluginSDK/Game.MW05/MW05.h>

#include <cmath>
#include <cstring>
#include <sstream>

namespace frr::game {
namespace {
frr::domain::PostRaceObserver observer;
std::uint64_t lastSample = 0;

// POD-only SEH boundary. No stored pointer is ever read on a later frame.
bool readVehicle(void* raw, frr::domain::ObservedRaceVehicle& out, bool& ignored) {
    ignored = false;
#if defined(_MSC_VER)
    __try {
#endif
        using namespace NFSPluginSDK::MW05;
        if (!raw) return false;
        auto* v = reinterpret_cast<IVehicle*>(raw);
        if (!v->IsActive() || v->IsDestroyed()) { ignored = true; return true; }
        if (v->IsLoading()) return false;
        auto* simable = v->GetSimable();
        if (!simable) return false;
        auto* body = simable->GetRigidBody();
        if (!body) return false;
        const auto pos = canonicalMwVector(body->GetPosition());
        if (!std::isfinite(pos.x) || !std::isfinite(pos.y) || !std::isfinite(pos.z)) return false;
        out.vehicle = reinterpret_cast<std::uintptr_t>(raw);
        out.simable = reinterpret_cast<std::uintptr_t>(simable);
        out.model = v->GetVehicleKey();
        out.driverClass = static_cast<std::uint32_t>(mwsdk::mw05::driver_class(raw));
        out.racer = out.driverClass == static_cast<std::uint32_t>(mwsdk::mw05::DriverClass::Racer);
        out.x = pos.x; out.y = pos.y; out.z = pos.z;
        // Read interface identity only. No guessed AI layout, GetGoalName call,
        // goal change, race-table edit, or PVehicle pointer subtraction.
        auto* ai = v->GetAIVehiclePtr();
        out.ai = reinterpret_cast<std::uintptr_t>(ai);
        if (ai) std::memcpy(&out.aiInterfaceVtable, ai, sizeof(out.aiInterfaceVtable));
        return out.model != 0;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}

// The registry access itself also needs a POD-only SEH boundary. Read the
// existing pinned SDK's data symbol, without introducing a guessed address.
bool readHeader(std::uint32_t& count, std::uintptr_t& storage) {
#if defined(_MSC_VER)
    __try {
#endif
        count = mwsdk::mw05::vehicle_count();
        storage = reinterpret_cast<std::uintptr_t>(mwsdk::mw05::read<void**>(
            mwsdk::mw05::process(), mwsdk::mw05::db::data::PVehicle_mVehicleListData));
        return count <= 512 && (!count || storage);
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
bool readIdentity(std::uint32_t index, std::uintptr_t& identity) {
#if defined(_MSC_VER)
    __try {
#endif
        identity = reinterpret_cast<std::uintptr_t>(mwsdk::mw05::vehicle_at(index));
        return identity != 0;
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
bool readSlot(std::uint32_t index, frr::domain::ObservedRaceVehicle& out,
    bool& ignored, std::uintptr_t& identity) {
#if defined(_MSC_VER)
    __try {
#endif
        if (!readIdentity(index, identity)) return false;
        return readVehicle(reinterpret_cast<void*>(identity), out, ignored);
#if defined(_MSC_VER)
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
#endif
}
}

void samplePostRaceRacers() {
    const auto loop = GameplayLoopHook::snapshot();
    if (!loop.installed || !loop.sourceVerified || !loop.threadConsistent ||
        !loop.completed || loop.threadId != GetCurrentThreadId()) {
        observer.reset();
        return;
    }
    const auto now = GetTickCount64();
    if (lastSample && now - lastSample < 500) return;
    lastSample = now;
    const auto current = GameBridge::sample();
    frr::domain::RaceVehicleObservation sample{};
    sample.millis = now;
    sample.context = {current.vehicles.playerIVehicle, current.roadNetwork,
        current.raceStatus, current.career.profileKeyAvailable ? current.career.profileKey : 0};
    if (!current.inWorld || current.inNIS || current.fadeScreen || current.raceStatusLoading ||
        !current.vehicles.independentPlayerCrossCheck) {
        observer.reset();
        std::ostringstream line;
        line << "PostRace observation revoked: inWorld=" << current.inWorld
            << " nis=" << current.inNIS << " fade=" << current.fadeScreen
            << " loading=" << current.raceStatusLoading
            << " playerCrossCheck=" << current.vehicles.independentPlayerCrossCheck
            << " readOnly=1 lifetimeProven=0";
        Log::instance().info(line.str());
        return;
    }
    if (current.mode == WorldProbeMode::StockRace)
        sample.phase = frr::domain::RaceObservationPhase::Racing;
    else if (current.mode == WorldProbeMode::FreeRoamCandidate)
        sample.phase = frr::domain::RaceObservationPhase::Roaming;
    frr::domain::VehicleRegistrySnapshot before{}, after{};
    before.complete = readHeader(before.count, before.storage);
    for (std::uint32_t i = 0; before.complete && i < before.count; ++i) {
        frr::domain::ObservedRaceVehicle vehicle{};
        bool ignored = false;
        std::uintptr_t identity = 0;
        before.complete = readSlot(i, vehicle, ignored, identity);
        if (before.complete) {
            before.slots.push_back(identity); // Include inactive slots in membership proof.
            if (!ignored) sample.vehicles.push_back(vehicle);
        }
    }
    after.complete = before.complete && readHeader(after.count, after.storage);
    for (std::uint32_t i = 0; after.complete && i < after.count; ++i) {
        std::uintptr_t identity = 0;
        after.complete = readIdentity(i, identity);
        if (after.complete) after.slots.push_back(identity);
    }
    // Guard the second traversal's header too. This still cannot detect an
    // unobserved remove/reinsert (ABA); logs deliberately keep lifetimeProven=0.
    std::uint32_t finalCount = 0;
    std::uintptr_t finalStorage = 0;
    after.complete = after.complete && readHeader(finalCount, finalStorage) &&
        finalCount == after.count && finalStorage == after.storage;
    const auto registryStatus = frr::domain::compareVehicleRegistrySnapshots(before, after);
    sample.complete = registryStatus == frr::domain::RegistrySnapshotStatus::Stable;
    const auto matches = observer.observe(sample);
    std::ostringstream summary;
    summary << "PostRace observation: phase=" << racePlayModeName(current.racePlayMode)
        << " complete=" << sample.complete << " liveCount=" << before.count
        << " registryStatus=" << frr::domain::registrySnapshotStatusName(registryStatus)
        << " matchingRaceIdentities=" << matches.size()
        << " readOnly=1 lifetimeProven=0";
    Log::instance().info(summary.str());
    for (const auto& match : matches) {
        const auto& v = match.current;
        std::ostringstream line;
        line << "PostRace identity match: iv=0x" << std::hex << v.vehicle
            << " simable=0x" << v.simable << " model=0x" << v.model
            << " ai=0x" << v.ai << " aiInterfaceVtable=0x" << v.aiInterfaceVtable << std::dec
            << " driverClass=" << v.driverClass << " roamingSamples=" << match.roamingSamples
            << " sinceRaceMs=" << match.millisSinceLastRaceSample
            << " x=" << v.x << " y=" << v.y << " z=" << v.z
            << " displacementAvailable=" << match.displacementAvailable
            << " displacementWorld=" << match.displacementSincePreviousRoamingSample
            << " aiIdentityChanged=" << match.aiIdentityChanged
            << " lifetimeProven=0";
        Log::instance().info(line.str());
    }
}
} // namespace frr::game
