#include "RuntimeProbe.h"

#include "GameBridge.h"
#include "../core/Log.h"

#include <nfsmw_sdk/input.h>

#include <cstdint>
#include <iomanip>
#include <sstream>
#include <string>

namespace frr::game {
namespace {

RuntimeProbeConfig g_config{};
RuntimeSnapshot g_last{};
bool g_haveLast = false;
std::uint64_t g_frame = 0;

bool sameMeaningfulState(
    const RuntimeSnapshot& a,
    const RuntimeSnapshot& b
) {
    return
        a.inNIS == b.inNIS &&
        a.fadeScreen == b.fadeScreen &&
        a.raceStatus == b.raceStatus &&
        a.vehicles.playerVehicle ==
            b.vehicles.playerVehicle &&
        a.vehicles.totalVehicles ==
            b.vehicles.totalVehicles &&
        a.vehicles.playerVehicles ==
            b.vehicles.playerVehicles &&
        a.vehicles.aiVehicles ==
            b.vehicles.aiVehicles &&
        a.mode == b.mode;
}

std::string describe(const RuntimeSnapshot& s) {
    std::ostringstream out;

    out << "mode=" << worldProbeModeName(s.mode)
        << " player=0x"
        << std::hex << std::uppercase
        << s.vehicles.playerVehicle
        << " raceStatus=0x"
        << s.raceStatus
        << " gameFlowRaw=0x"
        << s.gameFlowRaw
        << std::dec
        << " pvehicles=" << s.vehicles.totalVehicles
        << " playerCars=" << s.vehicles.playerVehicles
        << " aiCars=" << s.vehicles.aiVehicles
        << " unknownCars=" << s.vehicles.unknownVehicles
        << " inNIS=" << (s.inNIS ? 1 : 0)
        << " fade=" << (s.fadeScreen ? 1 : 0);

    return out.str();
}

void onRuntimeTick() {
    ++g_frame;

    const RuntimeSnapshot current = GameBridge::sample();

    const bool changed =
        !g_haveLast ||
        !sameMeaningfulState(current, g_last);

    const bool heartbeat =
        g_config.heartbeatFrames > 0 &&
        (g_frame % g_config.heartbeatFrames) == 0;

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

} // namespace

bool RuntimeProbe::install(
    const RuntimeProbeConfig& config
) {
    g_config = config;

    const bool installed =
        nfsmw::input::on_poll([]() {
            onRuntimeTick();
        });

    if (installed) {
        Log::instance().info(
            "Read-only runtime tick installed on the game's input poller."
        );
    } else {
        Log::instance().error(
            "Failed to install read-only runtime tick."
        );
    }

    return installed;
}

} // namespace frr::game
