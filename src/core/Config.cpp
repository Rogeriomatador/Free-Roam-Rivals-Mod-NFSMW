#include "Config.h"

#include <windows.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <vector>

namespace frr {
namespace {

std::filesystem::path executableDirectory() {
    std::vector<char> buffer(32768, '\0');
    const DWORD length = GetModuleFileNameA(
        nullptr,
        buffer.data(),
        static_cast<DWORD>(buffer.size())
    );

    if (length == 0 || length >= buffer.size()) {
        return std::filesystem::current_path();
    }

    return std::filesystem::path(
        std::string(buffer.data(), length)
    ).parent_path();
}

} // namespace

Config Config::load() {
    Config cfg{};

    const auto path =
        executableDirectory() /
        "scripts" /
        "FreeRoamRivals" /
        "FreeRoamRivals.ini";

    const std::string ini = path.string();

    cfg.runtimeProbeEnabled =
        GetPrivateProfileIntA(
            "Diagnostics",
            "RuntimeProbeEnabled",
            1,
            ini.c_str()
        ) != 0;

    const int heartbeat =
        GetPrivateProfileIntA(
            "Diagnostics",
            "RuntimeProbeHeartbeatFrames",
            600,
            ini.c_str()
        );

    cfg.runtimeProbeHeartbeatFrames =
        static_cast<unsigned>(
            std::clamp(heartbeat, 60, 36000)
        );

    return cfg;
}

} // namespace frr
