#include "RuntimeProbe.h"

#include "GameBridge.h"
#include "../core/Log.h"

#include <nfsmw_sdk/d3d9_hooks.h>
#include <nfsmw_sdk/input.h>

#include <windows.h>

#include <atomic>
#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace frr::game {
namespace {

RuntimeProbeConfig g_config{};
RuntimeSnapshot g_last{};
bool g_haveLast = false;

std::atomic<std::uint64_t> g_renderFrames{0};
std::atomic<std::uint64_t> g_inputPolls{0};
std::atomic<std::uint64_t> g_samples{0};

bool sameMeaningfulState(
    const RuntimeSnapshot& a,
    const RuntimeSnapshot& b
) {
    return
        a.inWorld == b.inWorld &&
        a.inNIS == b.inNIS &&
        a.fadeScreen == b.fadeScreen &&
        a.gameFlowState == b.gameFlowState &&
        a.raceStatus == b.raceStatus &&
        a.raceStatusLoading == b.raceStatusLoading &&
        a.racePlayMode == b.racePlayMode &&
        a.roadNetwork == b.roadNetwork &&
        a.vehicles.registryReadable ==
            b.vehicles.registryReadable &&
        a.vehicles.playerIVehicle ==
            b.vehicles.playerIVehicle &&
        a.vehicles.playerPVehicleCandidate ==
            b.vehicles.playerPVehicleCandidate &&
        a.vehicles.playerPVehicleVtableVerified ==
            b.vehicles.playerPVehicleVtableVerified &&
        a.vehicles.totalVehicles ==
            b.vehicles.totalVehicles &&
        a.vehicles.humanVehicles ==
            b.vehicles.humanVehicles &&
        a.vehicles.trafficVehicles ==
            b.vehicles.trafficVehicles &&
        a.vehicles.copVehicles ==
            b.vehicles.copVehicles &&
        a.vehicles.racerVehicles ==
            b.vehicles.racerVehicles &&
        a.career.available == b.career.available &&
        a.career.cash == b.career.cash &&
        a.career.careerCars == b.career.careerCars &&
        a.career.currentCarHandle ==
            b.career.currentCarHandle &&
        a.career.careerCompletedAtLeastOnce ==
            b.career.careerCompletedAtLeastOnce &&
        a.mode == b.mode;
}

std::string describe(const RuntimeSnapshot& s) {
    std::ostringstream out;

    out << "mode=" << worldProbeModeName(s.mode)
        << " inWorld=" << (s.inWorld ? 1 : 0)
        << " raceMode=" << racePlayModeName(s.racePlayMode)
        << " loading=" << (s.raceStatusLoading ? 1 : 0)
        << " inNIS=" << (s.inNIS ? 1 : 0)
        << " fade=" << (s.fadeScreen ? 1 : 0)
        << " flow=" << s.gameFlowState
        << " IVehicle=0x"
        << std::hex << std::uppercase
        << s.vehicles.playerIVehicle
        << " PVehicleCandidate=0x"
        << s.vehicles.playerPVehicleCandidate
        << " raceStatus=0x"
        << s.raceStatus
        << " roadNetwork=0x"
        << s.roadNetwork
        << std::dec
        << " pvehicleVerified="
        << (s.vehicles.playerPVehicleVtableVerified ? 1 : 0)
        << " vehicles=" << s.vehicles.totalVehicles
        << " human=" << s.vehicles.humanVehicles
        << " traffic=" << s.vehicles.trafficVehicles
        << " cops=" << s.vehicles.copVehicles
        << " racers=" << s.vehicles.racerVehicles
        << " none=" << s.vehicles.noneVehicles
        << " nisCars=" << s.vehicles.nisVehicles
        << " remote=" << s.vehicles.remoteVehicles
        << " unknown=" << s.vehicles.unknownVehicles;

    if (s.career.available) {
        out << " cash=" << s.career.cash
            << " careerCars=" << s.career.careerCars
            << " currentCarHandle="
            << s.career.currentCarHandle
            << " careerCompleted="
            << (s.career.careerCompletedAtLeastOnce ? 1 : 0);
    } else {
        out << " career=unavailable";
    }

    return out.str();
}

void sampleAndLog(
    std::uint64_t renderFrame
) {
    const RuntimeSnapshot current = GameBridge::sample();
    ++g_samples;

    const bool changed =
        !g_haveLast ||
        !sameMeaningfulState(current, g_last);

    const bool heartbeat =
        g_config.heartbeatFrames > 0 &&
        (renderFrame % g_config.heartbeatFrames) == 0;

    if (changed) {
        Log::instance().info(
            std::string("Runtime state changed: ") +
            describe(current)
        );
    } else if (heartbeat) {
        Log::instance().info(
            std::string("Runtime heartbeat: ") +
            describe(current)
        );
    }

    g_last = current;
    g_haveLast = true;
}

void onRenderFrame(void*) {
    const std::uint64_t frame = ++g_renderFrames;

    if (g_config.sampleEveryFrames == 0 ||
        frame == 1 ||
        (frame % g_config.sampleEveryFrames) == 0) {
        sampleAndLog(frame);
    }
}

void onInputPoll() {
    ++g_inputPolls;
}

DWORD WINAPI healthThread(LPVOID) {
    // We only inspect our own atomics here. No engine objects are
    // dereferenced from this background thread.
    Sleep(8000);

    std::ostringstream out;
    out << "Runtime hook health after 8s:"
        << " renderFrames=" << g_renderFrames.load()
        << " inputPolls=" << g_inputPolls.load()
        << " samples=" << g_samples.load();

    Log::instance().info(out.str());

    if (g_renderFrames.load() == 0) {
        Log::instance().warn(
            "D3D9 EndScene callback has not fired yet."
        );
    }

    if (g_inputPolls.load() == 0) {
        Log::instance().warn(
            "Input-poll callback has not fired yet; it is diagnostic-only in v0.0.5-dev."
        );
    }

    return 0;
}

} // namespace

RuntimeProbeInstallResult RuntimeProbe::install(
    const RuntimeProbeConfig& config
) {
    g_config = config;

    RuntimeProbeInstallResult result{};

    if (config.renderProbeEnabled) {
        nfsmw_d3d9_install(&onRenderFrame, nullptr);
        result.renderProbeArmed = true;

        Log::instance().info(
            "D3D9 EndScene runtime probe armed (read-only)."
        );
    }

    if (config.inputProbeEnabled) {
        result.inputProbeInstalled =
            nfsmw::input::on_poll([]() {
                onInputPoll();
            });

        if (result.inputProbeInstalled) {
            Log::instance().info(
                "Input-poll diagnostic hook installed."
            );
        } else {
            Log::instance().warn(
                "Input-poll diagnostic hook failed to install."
            );
        }
    }

    HANDLE thread = CreateThread(
        nullptr,
        0,
        &healthThread,
        nullptr,
        0,
        nullptr
    );

    if (thread) {
        CloseHandle(thread);
    }

    return result;
}

} // namespace frr::game
