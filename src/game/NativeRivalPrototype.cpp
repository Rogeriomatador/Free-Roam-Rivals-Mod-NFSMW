#include "NativeRivalPrototype.h"
#include "NativeVehicleFactory.h"
#include "NativeEncounter.h"
#include "ChallengeInputProbe.h"
#include "GameBridge.h"
#include "GameplayLoopHook.h"
#include "CameraFrustumProbe.h"
#include "VehicleCatalogProbe.h"
#include "VehicleSpatialProbe.h"
#include "WorldCollisionProbe.h"
#include "../core/Log.h"
#include "../core/VersionGuard.h"
#include "../domain/NativeSearchWindow.h"
#include "../domain/PrototypeGroundProbe.h"
#include <windows.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <sstream>
#include <string>

namespace frr::game {
namespace {
using namespace domain;
enum class Stage { Idle, Seeking, Confirming, Loading, PreparingRacer, PreparingRoad, Activating, Active, Retiring, Finished, Disabled };
Stage stage = Stage::Idle;
bool nearPlayerDebug = false;
bool cleanupAfterConfirmation = false;
bool enabled = false, keyWasDown = false, attemptSpent = false;
std::uint64_t lastTick = 0, stageStarted = 0, lastObservation = 0, lastBlockLog = 0;
unsigned stable = 0;
std::uintptr_t lastPlayer = 0, lastRoad = 0, lastRace = 0;
std::uint64_t lastProfile = 0;
std::array<std::size_t, 32> candidateCursors{};
std::uint64_t lastSearchReport = 0;
std::string lastBlock;
float cleanupHold=0, encounterDelta=0;
bool cleanupLatched=false, challengePending=false;
SpatialVector3 startPosition{};
NativeRoadTarget selectedTarget{};

const char* stageName(Stage s) {
    switch (s) {
        case Stage::Idle: return "idle"; case Stage::Seeking: return "seeking";
        case Stage::Confirming: return "confirming_construction";
        case Stage::Loading: return "loading"; case Stage::PreparingRacer: return "preparing_racer";
        case Stage::PreparingRoad: return "preparing_road"; case Stage::Activating: return "activating";
        case Stage::Active: return "active"; case Stage::Retiring: return "retiring";
        case Stage::Finished: return "finished"; default: return "disabled";
    }
}
void transition(Stage next) {
    if (next!=Stage::Active) { interruptNativeEncounter(); encounterDelta=0; challengePending=false; }
    stage = next; stageStarted = GetTickCount64(); lastBlock.clear();
    Log::instance().info(std::string("NativePrototype stage=") + stageName(next) + " maxOwned=1 gameplayCallback=1");
}
void blocked(const std::string& reason) {
    showNativeRivalStatus("AGUARDANDO CONDICOES SEGURAS", stage==Stage::Seeking ? "DIRIJA O GOLF GTI E TENTE NOVAMENTE" : "RIVAL PRESERVADO DURANTE A TRANSICAO");
    const auto now = GetTickCount64();
    if (reason != lastBlock || now-lastBlockLog >= 5000) {
        Log::instance().warn(std::string("NativePrototype blocked=") + reason + " stage=" + stageName(stage));
        lastBlock = reason; lastBlockLog = now;
    }
}
float distance(SpatialVector3 a, SpatialVector3 b) {
    const float dx=a.x-b.x, dy=a.y-b.y, dz=a.z-b.z;
    return std::sqrt(dx*dx+dy*dy+dz*dz);
}
SpatialVector3 add(SpatialVector3 a, SpatialVector3 b, float scale) {
    return {a.x+b.x*scale, a.y+b.y*scale, a.z+b.z*scale};
}
VehicleOrientedBox conservativeBox(const NativeRoadTarget& target, std::uint32_t key) {
    SpatialVector3 forward{target.forward.x,target.forward.y,target.forward.z};
    const float horizontal = std::sqrt(forward.x*forward.x + forward.z*forward.z);
    if (!std::isfinite(horizontal) || horizontal < 0.5f) return {};
    SpatialVector3 right{forward.z/horizontal,0,-forward.x/horizontal};
    SpatialVector3 up{forward.y*right.z, forward.z*right.x-forward.x*right.z, -forward.y*right.x};
    return makeVehicleOrientedBox(0, key,
        {target.position.x,target.position.y+2.0f,target.position.z}, right,up,forward,{3.0f,2.0f,8.0f});
}
bool groundSafe(const VehicleOrientedBox& box) {
    if (!box.valid) return false;
    for (int i=0; i<5; ++i) {
        auto point = add(box.center, box.up, -box.halfExtents.y);
        if (i) {
            point=add(point,box.right,(i<=2 ? -1.0f : 1.0f)*box.halfExtents.x);
            point=add(point,box.forward,(i%2 ? -1.0f : 1.0f)*box.halfExtents.z);
        }
        const auto sample=WorldCollisionProbe::samplePrototypeGround(point);
        const auto ground=interpretGroundCollision(point,sample);
        if (!prototypeGroundAcceptable(ground)) {
            static std::uint64_t lastGroundLog = 0;
            const auto now=GetTickCount64();
            if (now-lastGroundLog>=1000) {
                lastGroundLog=now;
                std::ostringstream line;
                line << "NativePrototype ground rejected: footprintPoint=" << i << " available=" << sample.callAvailable
                    << " completed=" << sample.callCompleted << " hit=" << sample.hit << " type=" << unsigned(sample.hitType)
                    << " normalY=" << ground.normal.y << " heightDelta=" << ground.absoluteHeightDeltaWorldUnits
                    << " grade=" << ground.absoluteGrade << " candidateY=" << point.y << " hitY=" << sample.hitPoint.y;
                Log::instance().warn(line.str());
            }
            return false;
        }
    }
    return true;
}
bool hiddenForPrototype(const VehicleOrientedBox& box, const RuntimeSnapshot& world,
    const WorldMetricCalibration& metric) {
    const auto camera=CameraFrustumProbe::sample();
    const auto visibility=classifyPrimaryCameraFootprint(camera,box);
    if (!visibility.queryVerified || visibility.visibility!=CameraBoxVisibility::OutsideFrustum) return false;
    const SpatialVector3 eye{-camera.eyeRender.y,camera.eyeRender.z,camera.eyeRender.x};
    const auto& p=world.playerMotion.position;
    const auto cameraDistance=worldUnitsToMeters(distance(eye,{p.x,p.y,p.z}),metric);
    if (!cameraDistance || *cameraDistance>30.0f) return false;
    const auto margin=metersToWorldUnits(2.0f,metric);
    if (!margin) return false;
    // Prototype-specific conservative checks, NOT promotion of the domain's
    // spawnVisibilityVerified flag or proof of mirrors/shadows/all render views.
    for (int i=0; i<8; ++i) {
        auto corner=add(box.center,box.right,(i&1 ? 1.0f : -1.0f)*box.halfExtents.x);
        corner=add(corner,box.up,(i&2 ? 1.0f : -1.0f)*box.halfExtents.y);
        corner=add(corner,box.forward,(i&4 ? 1.0f : -1.0f)*box.halfExtents.z);
        const auto sample=WorldCollisionProbe::sampleWorldOcclusion(eye,corner);
        if (!sample.callAvailable || !sample.callCompleted || !sample.hit || sample.hitType!=1 ||
            !std::isfinite(sample.hitPoint.x) || !std::isfinite(sample.hitPoint.y) || !std::isfinite(sample.hitPoint.z) ||
            distance(eye,sample.hitPoint)+*margin>=distance(eye,corner)) return false;
    }
    return true;
}
SpawnEnvironmentInput environment(const RuntimeSnapshot& current, PursuitSafetyState pursuit, bool ownedPresent) {
    SpawnEnvironmentInput out{};
    out.nearPlayerDebugRequested=nearPlayerDebug;
    out.experimentalFeatureEnabled=true; out.supportedExecutable=true;
    out.freeRoamCandidate=current.mode==WorldProbeMode::FreeRoamCandidate;
    out.loading=current.raceStatusLoading; out.inNIS=current.inNIS; out.fade=current.fadeScreen;
    out.playerAvailable=current.vehicles.playerIVehicle!=0;
    out.independentPlayerCrossCheck=current.vehicles.independentPlayerCrossCheck;
    out.roadNetworkAvailable=current.roadNetwork!=0; out.pursuitState=pursuit;
    out.stableFreeRoamSamples=stable;
    out.liveRivals=static_cast<int>(current.vehicles.racerVehicles)-(ownedPresent ? 1 : 0);
    out.liveRivals=std::max(out.liveRivals,0); out.maxLiveRivals=1;
    return out;
}
bool candidateSafe(const VehicleOrientedBox& box, const RuntimeSnapshot& current,
    const WorldMetricCalibration& metric, SpawnCandidateInput& out) {
    out={};
    if (!box.valid || !validWorldMetricCalibration(metric) || !current.playerMotion.position.finite) {
        blocked("geometry_or_metric_unavailable"); return false;
    }
    const auto& p=current.playerMotion.position;
    const auto meters=worldUnitsToMeters(distance(box.center,{p.x,p.y,p.z}),metric);
    const float minDistance=nearPlayerDebug ? 20.0f : 350.0f;
    const float maxDistance=nearPlayerDebug ? 120.0f : 850.0f;
    if (!meters || !std::isfinite(*meters) || *meters<minDistance || *meters>maxDistance) {
        blocked(nearPlayerDebug ? "candidate_distance_debug_20_to_120m" : "candidate_distance_350_to_850m"); return false;
    }
    const auto fleet=VehicleSpatialProbe::sample();
    const auto overlap=evaluateFootprintAgainstFleet(box,fleet.boxes,fleet.registryComplete,
        metric.worldUnitsPerMeter);
    if (!overlap.verified || overlap.overlaps) { blocked("fresh_fleet_clearance"); return false; }
    if (!groundSafe(box)) { blocked("ground_under_full_footprint"); return false; }
    if (!nearPlayerDebug && !hiddenForPrototype(box,current,metric)) { blocked("primary_camera_and_eight_world_rays"); return false; }
    out.available=true; out.vehicleAvailable=true; out.roadValid=true; out.groundValid=true;
    out.overlapsLiveVehicle=false; out.visibleToPlayer=nearPlayerDebug; // Unknown/visible in debug mode, never claim hidden.
    out.metricDistanceVerified=true; out.distanceFromPlayerMeters=*meters;
    // Collision/graph access does not prove render/model streaming. No promotion.
    out.streamingVerified=false;
    return true;
}
void beginCleanup(const char* reason) {
    Log::instance().warn(std::string("NativePrototype cleanup requested reason=")+reason);
    transition(Stage::Retiring);
}
void mutationResult(NativeFactoryResult result, NativeFactoryResult expected, Stage next) {
    if (result==expected) transition(next);
    else if (result==NativeFactoryResult::CompatibilityBlocked) {
        Log::instance().warn("NativePrototype compatibility audit failed; no constructor called; restart after resolving the reported code difference.");
        transition(Stage::Disabled);
    }
    else if (result==NativeFactoryResult::Faulted) {
        if (NativeVehicleFactory::snapshot().owned) beginCleanup("native_operation_failed");
        else transition(Stage::Disabled);
    } else blocked("native_operation_rejected");
}
}

void configureNativeRivalPrototype(bool requested, bool nearPlayer) {
    nearPlayerDebug=requested && nearPlayer;
    enabled=requested && VersionGuard::checkCurrentExecutable().supported;
    if (enabled) {
        unsigned char bytes[256]{}; SIZE_T got=0;
        std::uint64_t hash=14695981039346656037ull;
        if (!ReadProcessMemory(GetCurrentProcess(),reinterpret_cast<void*>(0x7854B0),bytes,sizeof(bytes),&got) || got!=sizeof(bytes)) enabled=false;
        else { for (auto byte:bytes) hash=(hash^byte)*1099511628211ull; enabled=hash==0x9e1818df51dff4b3ull; }
    }
    if (enabled && nearPlayerDebug) Log::instance().warn("NativePrototype near-player debug enabled: visible creation allowed at 20-120m; road, ground, clearance and pursuit checks retained; cleanup still hidden/300m.");
    if (requested) Log::instance().info(enabled ?
        "NativePrototype armed: F8 requests one stock Golf GTI; hold F7 for 1.5 seconds to request safe cleanup. Native roaming driving observed in v41; challenge integration requires in-game validation." :
        "NativePrototype blocked: exact supported executable and unchanged world-collision entry required.");
}
void tickNativeRivalPrototype(const WorldMetricCalibration& metric, float updateDelta) {
    if (!enabled || !GameplayLoopHook::isInAfterCallback()) return;
    const auto loop=GameplayLoopHook::snapshot();
    if (!loop.installed || !loop.sourceVerified || !loop.threadConsistent || loop.threadId!=GetCurrentThreadId()) return;
    const bool down=(GetAsyncKeyState(VK_F8)&0x8000)!=0;
    DWORD foregroundProcess=0;
    GetWindowThreadProcessId(GetForegroundWindow(),&foregroundProcess);
    const bool pressed=down&&!keyWasDown&&foregroundProcess==GetCurrentProcessId()&&
        std::isfinite(updateDelta)&&updateDelta>0;
    keyWasDown=down;
    bool challengeEdge=false;
    while (ChallengeInputProbe::consumePress()) challengeEdge=true;
    if (!std::isfinite(updateDelta) || updateDelta<=0) { challengePending=false; return; }
    const bool focused=foregroundProcess==GetCurrentProcessId();
    if (stage==Stage::Active) {
        encounterDelta+=updateDelta;
        challengePending=challengePending || (challengeEdge && focused);
    }
    const bool cleanupDown=focused && (GetAsyncKeyState(VK_F7)&0x8000)!=0;
    if (!cleanupDown) { cleanupHold=0; cleanupLatched=false; }
    else if (!cleanupLatched) {
        // Large/invalid deltas cannot turn a single press into a removal.
        cleanupHold+=std::min(updateDelta,0.1f);
        if (cleanupHold>=1.5f) {
            cleanupLatched=true;
            if (stage==Stage::Confirming) cleanupAfterConfirmation=true;
            else if (stage==Stage::Seeking) transition(Stage::Idle);
            else if (stage==Stage::Active || stage==Stage::Loading || stage==Stage::PreparingRacer ||
                stage==Stage::PreparingRoad || stage==Stage::Activating) beginCleanup("F7_held_1_5_seconds");
        }
    }
    if (pressed) {
        if (stage==Stage::Idle && !attemptSpent) transition(Stage::Seeking);
        else if (stage==Stage::Seeking) blocked("search_already_in_progress_wait_10_seconds");
        else Log::instance().info("NativePrototype F8 ignored: existing rival retained; hold F7 for explicit safe retirement.");
    }
    const auto now=GetTickCount64();
    // Confirmation follows completed gameplay frames, not the 250ms search
    // throttle. Keep the native vehicle's unprepared interval to two frames.
    if (stage==Stage::Confirming) {
        const auto result=NativeVehicleFactory::confirmConstructionInactive();
        if (result==NativeFactoryResult::ConstructedInactive) {
            lastTick=now;
            if (cleanupAfterConfirmation) beginCleanup("F7_after_identity_confirmation");
            else transition(Stage::Loading);
        }
        else if (result==NativeFactoryResult::Faulted) mutationResult(result,NativeFactoryResult::ConstructedInactive,Stage::Loading);
        else blocked("waiting_for_two_completed_registry_confirmations");
        return;
    }
    if (lastTick && now-lastTick<250) return;
    lastTick=now;
    // No native factory/world reads while idle, including startup menus.
    // A new F8 request builds its own fresh stable-clear window.
    if (stage==Stage::Idle || stage==Stage::Finished || stage==Stage::Disabled) {
        stable=0; showNativeRivalStatus(stage==Stage::Idle ? "F8: CRIAR RICO NO MUNDO" : "RIVAL ENCERRADO NESTA SESSAO", "USE O GOLF GTI E DIRIJA PARA CALIBRAR"); return;
    }
    const auto current=GameBridge::sample();
    const auto pursuit=NativeVehicleFactory::pursuitState();
    const auto profile=current.career.profileKeyAvailable ? current.career.profileKey : 0;
    const bool sameContext=lastPlayer==current.vehicles.playerIVehicle && lastRoad==current.roadNetwork &&
        lastRace==current.raceStatus && lastProfile==profile;
    if (sameContext && current.mode==WorldProbeMode::FreeRoamCandidate && pursuit==PursuitSafetyState::Clear) stable=std::min(stable+1u,6u);
    else stable=0;
    lastPlayer=current.vehicles.playerIVehicle; lastRoad=current.roadNetwork;
    lastRace=current.raceStatus; lastProfile=profile;
    if (stage==Stage::Seeking && now-stageStarted>10000) {
        blocked("no_safe_candidate_within_request_window_press_F8_to_retry"); transition(Stage::Idle); return;
    }
    const auto owned=NativeVehicleFactory::snapshot();
    if (owned.owned && !owned.contextMatches) {
        interruptNativeEncounter(); encounterDelta=0; challengePending=false;
        blocked("world_transition_no_old_pointer_reads"); return;
    }
    if (stage==Stage::Active) {
        if (!owned.available || owned.destroyed) {
            const auto external=NativeVehicleFactory::observeExternalRemoval();
            if (external==NativeFactoryResult::RemovedByEngine) {
                Log::instance().warn("NativePrototype native engine retired the vehicle; mod cleanup was not executed; lifetimeProven=0");
                transition(Stage::Disabled);
            } else if (external==NativeFactoryResult::RemovalPending) blocked("confirming_external_native_retirement");
            else blocked("owned_vehicle_unavailable_wait_without_cleanup");
            interruptNativeEncounter(); encounterDelta=0; challengePending=false;
            return;
        }
        tickNativeEncounter(current,owned,metric,encounterDelta,challengePending);
        encounterDelta=0; challengePending=false;
        if (!owned.active) blocked("native_vehicle_inactive_no_forced_reactivation");
        if (now-lastObservation>=1000) {
            lastObservation=now;
            const auto traveled=worldUnitsToMeters(distance(startPosition,owned.box.center),metric);
            std::ostringstream line;
            line<<"NativePrototype observation: active="<<owned.active<<" model=0x"<<std::hex<<owned.box.vehicleKey<<std::dec
                <<" x="<<owned.box.center.x<<" y="<<owned.box.center.y<<" z="<<owned.box.center.z
                <<" speedMps="<<owned.speed<<" displacementMeters="<<(traveled ? *traveled : -1)
                <<" movementObserved="<<(traveled && *traveled>=5)<<" playerPursuitState="<<static_cast<int>(pursuit)
                <<" lifecycleProven=0";
            Log::instance().info(line.str());
        }
        return;
    }
    if (pursuit!=PursuitSafetyState::Clear || current.mode!=WorldProbeMode::FreeRoamCandidate) {
        stable=0; blocked("pursuit_cooldown_or_world_context_unsafe"); return;
    }
    if (!validWorldMetricCalibration(metric)) { blocked("drive_to_obtain_metric_calibration"); return; }
    if (stage==Stage::Seeking) {
        const auto key=VehicleCatalogProbe::runtimeKeyForName("gti");
        if (!key || current.playerMotion.vehicleKey!=*key) {
            blocked("select_Golf_GTI_for_first_prototype_shared_model_resources"); return;
        }
        const auto env=environment(current,pursuit,false);
        const auto readiness=evaluateSpawnEnvironment(env);
        if (!readiness.allowed) { blocked(spawnRejectReasonName(readiness.reason)); return; }
        std::size_t batchIndex = 0;
        NativeRoadCaptureReport capture{};
        const auto targets = NativeVehicleFactory::captureRoadTargets(batchIndex, capture);
        std::vector<std::size_t> eligible;
        for (std::size_t i = 0; i < targets.size(); ++i) {
            const auto& target = targets[i];
            const auto box=conservativeBox(target,*key);
            const auto& p=current.playerMotion.position;
            const auto d=worldUnitsToMeters(distance(box.center,{p.x,p.y,p.z}),metric);
            if (!d || !std::isfinite(*d) || *d<(nearPlayerDebug ? 20.0f : 350.0f) ||
                *d>(nearPlayerDebug ? 120.0f : 850.0f)) continue;
            eligible.push_back(i);
        }
        if (nearPlayerDebug) {
            const auto& p=current.playerMotion.position;
            const SpatialVector3 player{p.x,p.y,p.z};
            std::stable_sort(eligible.begin(),eligible.end(),[&](std::size_t a,std::size_t b) {
                return distance(conservativeBox(targets[a],*key).center,player)<
                    distance(conservativeBox(targets[b],*key).center,player);
            });
        }
        if (batchIndex >= candidateCursors.size()) { blocked("invalid_source_batch"); return; }
        const auto window = nextNativeSearchWindow(eligible.size(), 4, candidateCursors[batchIndex]);
        if (now-lastSearchReport >= 1000) {
            lastSearchReport = now;
            std::ostringstream line;
            line << "NativePrototype search: sourceBatch=" << batchIndex << " capturedTargets=" << targets.size()
                << " distanceEligible=" << eligible.size() << " safetyChecks=" << window.count
                << " candidateStart=" << window.start << " captureStatus=" << capture.status
                << " liveSlots=" << capture.liveSlots << " sampledSlots=" << capture.sampledSlots
                << " rejectVehicle=" << capture.rejected[1] << " rejectDriver=" << capture.rejected[2]
                << " rejectAI=" << capture.rejected[3] << " rejectNavPointer=" << capture.rejected[4]
                << " rejectNavMemory=" << capture.rejected[5] << " rejectSeed=" << capture.rejected[6]
                << " rejectChanged=" << capture.rejected[7] << " rejectGeometry=" << capture.rejected[8]
                << " rejectFault=" << capture.rejected[9] << " rejectContext=" << capture.rejectedContext;
            Log::instance().info(line.str());
        }
        for (std::size_t i = 0; i < window.count; ++i) {
            const auto& target = targets[eligible[window.index(i)]];
            const auto box=conservativeBox(target,*key);
            SpawnCandidateInput candidate{};
            if (!candidateSafe(box,current,metric,candidate)) continue;
            NativeFactoryRequest request{};
            request.environment=env; request.candidate=candidate; request.vehicleKey=*key;
            request.roadTarget=target; request.position=target.position; request.forward=target.forward;
            selectedTarget=target;
            std::ostringstream creation;
            creation << "NativePrototype native construction request: stock GTI, owned scalar road seed, inactive staging"
                << " nearPlayerDebug=" << nearPlayerDebug << " distanceMeters=" << candidate.distanceFromPlayerMeters
                << " x=" << target.position.x << " y=" << target.position.y << " z=" << target.position.z;
            Log::instance().info(creation.str());
            const auto result=NativeVehicleFactory::constructInactive(request);
            attemptSpent=result!=NativeFactoryResult::Blocked;
            mutationResult(result,NativeFactoryResult::ConstructionPending,Stage::Confirming);
            return;
        }
        if (eligible.empty()) blocked("no_distance_eligible_target_in_current_batch");
        return;
    }
    if (stage==Stage::Retiring) {
        if (stable<6) { blocked("cleanup_requires_stable_clear_pursuit_window"); return; }
        const auto observed=NativeVehicleFactory::observeRemoval();
        if (observed==NativeFactoryResult::Removed) { transition(Stage::Finished); return; }
        if (observed==NativeFactoryResult::RemovalPending) { blocked("waiting_for_both_native_registries"); return; }
        const auto external=NativeVehicleFactory::observeExternalRemoval();
        if (external==NativeFactoryResult::RemovedByEngine) {
            Log::instance().warn("NativePrototype native engine retired the vehicle before mod cleanup; lifetimeProven=0");
            transition(Stage::Disabled); return;
        }
        if (external==NativeFactoryResult::RemovalPending) { blocked("confirming_external_native_retirement"); return; }
        if (!owned.available || owned.loading || !owned.box.valid) { blocked("cleanup_requires_fresh_loaded_identity"); return; }
        const auto& p=current.playerMotion.position;
        const auto d=worldUnitsToMeters(distance(owned.box.center,{p.x,p.y,p.z}),metric);
        if (!d || *d<300 || !hiddenForPrototype(owned.box,current,metric)) {
            blocked("cleanup_deferred_until_hidden_and_300m_away"); return;
        }
        Log::instance().info("NativePrototype native retirement begin: owned simable only");
        const auto result=NativeVehicleFactory::requestRemoval();
        if (result!=NativeFactoryResult::RemovalRequested) blocked("cleanup_native_request_rejected");
        return;
    }
    if (!owned.available || owned.loading) {
        if (now-selectedTarget.millis>4000) beginCleanup("loading_timeout");
        else blocked("waiting_for_native_model_loading");
        return;
    }
    if (now-selectedTarget.millis>4500) { beginCleanup("staging_or_road_seed_expired"); return; }
    if (stage==Stage::Loading) { transition(Stage::PreparingRacer); return; }
    if (stage==Stage::PreparingRacer) {
        Log::instance().info("NativePrototype native Racer driver/goal preparation begin");
        mutationResult(NativeVehicleFactory::prepareRacerInactive(),NativeFactoryResult::RacerPreparedInactive,Stage::PreparingRoad);
        return;
    }
    if (stage==Stage::PreparingRoad) {
        Log::instance().info("NativePrototype native reset to owned scalar road seed begin");
        mutationResult(NativeVehicleFactory::resetRoadInactive(),NativeFactoryResult::RoadPreparedInactive,Stage::Activating);
        return;
    }
    if (stage==Stage::Activating) {
        if (!owned.box.valid || owned.box.halfExtents.x>3 || owned.box.halfExtents.y>2 || owned.box.halfExtents.z>8) {
            beginCleanup("actual_dimensions_exceed_reserved_footprint"); return;
        }
        SpawnCandidateInput candidate{};
        if (!candidateSafe(owned.box,current,metric,candidate)) { beginCleanup("activation_safety_revalidation_failed"); return; }
        startPosition=owned.box.center;
        Log::instance().info("NativePrototype activation begin: SetSpawned then native Racer goal then Activate");
        mutationResult(NativeVehicleFactory::activatePrepared(environment(current,pursuit,true),candidate),
            NativeFactoryResult::Activated,Stage::Active);
    }
}
}
