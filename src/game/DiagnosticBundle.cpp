#include "DiagnosticBundle.h"
#include "NativeVehicleFactory.h"
#include "GameplayLoopHook.h"
#include "GameBridge.h"
#include "VehicleSpatialProbe.h"
#include "../core/Log.h"
#include <windows.h>
#include <tlhelp32.h>
#include <cmath>
#include <sstream>

namespace frr::game {
void logDiagnosticModuleInventory(const char* reason) {
    const auto snapshot=CreateToolhelp32Snapshot(TH32CS_SNAPMODULE,GetCurrentProcessId());
    if (snapshot==INVALID_HANDLE_VALUE) {
        Log::instance().warn("DiagnosticBundle module inventory unavailable error="+std::to_string(GetLastError())); return;
    }
    MODULEENTRY32W entry{}; entry.dwSize=sizeof(entry);
    bool more=Module32FirstW(snapshot,&entry)!=FALSE;
    unsigned count=0;
    while (more && count<256) {
        char name[1024]{};
        WideCharToMultiByte(CP_UTF8,0,entry.szModule,-1,name,sizeof(name),nullptr,nullptr);
        std::ostringstream line;
        line<<"DiagnosticBundle module: name="<<name<<" base=0x"<<std::hex
            <<reinterpret_cast<std::uintptr_t>(entry.modBaseAddr)<<std::dec<<" bytes="<<entry.modBaseSize;
        Log::instance().info(line.str());
        ++count; more=Module32NextW(snapshot,&entry)!=FALSE;
    }
    const auto error=GetLastError();
    CloseHandle(snapshot);
    std::ostringstream line;
    line<<"DiagnosticBundle module inventory: reason="<<reason<<" count="<<count
        <<" truncated="<<more<<" complete="<<(!more && error==ERROR_NO_MORE_FILES);
    Log::instance().info(line.str());
}
void tickDiagnosticBundle(const domain::WorldMetricCalibration& metric, float updateDelta) {
    static bool keyWasDown=false;
    static std::uint64_t lastRequest=0;
    const bool down=(GetAsyncKeyState(VK_F9)&0x8000)!=0;
    DWORD foreground=0; GetWindowThreadProcessId(GetForegroundWindow(),&foreground);
    const bool pressed=down&&!keyWasDown&&foreground==GetCurrentProcessId(); keyWasDown=down;
    const auto loop=GameplayLoopHook::snapshot();
    if (!std::isfinite(updateDelta) || updateDelta<=0 ||
        !GameplayLoopHook::isInAfterCallback() || !loop.installed || !loop.sourceVerified ||
        !loop.threadConsistent || loop.threadId!=GetCurrentThreadId()) return;
    static bool initialInventoryDone=false;
    if (!initialInventoryDone) {
        initialInventoryDone=true; logDiagnosticModuleInventory("first_gameplay_callback");
        Log::instance().info("DiagnosticBundle ready: F9 requests bounded code/module/world audit; F8 prototype is a separate opt-in.");
    }
    if (!pressed) return;
    const auto now=GetTickCount64();
    if (lastRequest && now-lastRequest<10000) {
        Log::instance().warn("DiagnosticBundle F9 throttled: wait 10 seconds"); return;
    }
    lastRequest=now;
    Log::instance().info("DiagnosticBundle F9 begin: manual observation only; no vehicle creation or save writes");
    logDiagnosticModuleInventory("F9");
    const bool signatures=NativeVehicleFactory::auditCompatibilityReadOnly();
    const auto world=GameBridge::sample();
    std::ostringstream line;
    line<<"DiagnosticBundle world: mode="<<worldProbeModeName(world.mode)
        <<" flow="<<world.gameFlowState<<" loading="<<world.raceStatusLoading
        <<" NIS="<<world.inNIS<<" fade="<<world.fadeScreen
        <<" playerCrossCheck="<<world.vehicles.independentPlayerCrossCheck
        <<" live="<<world.vehicles.totalVehicles<<" physical="<<world.vehicles.pvehicleRegistryCount
        <<" traffic="<<world.vehicles.trafficVehicles<<" racers="<<world.vehicles.racerVehicles
        <<" cops="<<world.vehicles.copVehicles<<" playerModel=0x"<<std::hex<<world.playerMotion.vehicleKey
        <<std::dec<<" metricVerified="<<domain::validWorldMetricCalibration(metric)
        <<" worldUnitsPerMeter="<<metric.worldUnitsPerMeter<<" codeAuditPassed="<<signatures;
    Log::instance().info(line.str());
    // Avoid extra native/fleet/navigation queries in menus, transitions or pursuit.
    if (world.mode!=WorldProbeMode::FreeRoamCandidate || world.raceStatusLoading || world.inNIS || world.fadeScreen) {
        Log::instance().warn("DiagnosticBundle extended world sample skipped: unsafe/absent Free Roam context"); return;
    }
    const auto pursuit=NativeVehicleFactory::pursuitState();
    Log::instance().info("DiagnosticBundle pursuitState="+std::to_string(static_cast<int>(pursuit))+" (unknown=0 clear=1 active=2 cooldown=3 busted=4)");
    if (pursuit!=domain::PursuitSafetyState::Clear) {
        Log::instance().warn("DiagnosticBundle fleet/road sample skipped: pursuit not verified clear"); return;
    }
    const auto fleet=VehicleSpatialProbe::sample();
    std::ostringstream spatial;
    spatial<<"DiagnosticBundle fleet: complete="<<fleet.registryComplete<<" registered="<<fleet.registryCount
        <<" active="<<fleet.enabledActiveVehicles<<" ignoredInactive="<<fleet.ignoredInactiveVehicles
        <<" failed="<<fleet.failedSpatialReads<<" boxes="<<fleet.boxes.size();
    Log::instance().info(spatial.str());
    for (unsigned i=0;i<4;++i) {
        std::size_t batch=0; NativeRoadCaptureReport report{};
        const auto targets=NativeVehicleFactory::captureRoadTargets(batch,report);
        std::ostringstream road;
        road<<"DiagnosticBundle road: batch="<<batch<<" targets="<<targets.size()<<" status="<<report.status
            <<" live="<<report.liveSlots<<" sampled="<<report.sampledSlots<<" rejectContext="<<report.rejectedContext;
        for (unsigned j=1;j<10;++j) road<<" reject"<<j<<'='<<report.rejected[j];
        Log::instance().info(road.str());
    }
    Log::instance().info("DiagnosticBundle F9 end: four bounded road batches, not an exhaustive world scan; render/motion/lifetime unproven");
}
}
