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

int iniInt(
    const std::string& file,
    const char* key,
    int fallback
) {
    return GetPrivateProfileIntA(
        "Diagnostics",
        key,
        fallback,
        file.c_str()
    );
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

    cfg.renderProbeEnabled =
        iniInt(ini, "RenderProbeEnabled", 1) != 0;

    cfg.inputProbeEnabled =
        iniInt(ini, "InputProbeEnabled", 1) != 0;

    cfg.runtimeSampleEveryFrames =
        static_cast<unsigned>(
            std::clamp(
                iniInt(ini, "RuntimeSampleEveryFrames", 30),
                1,
                600
            )
        );

    cfg.runtimeProbeHeartbeatFrames =
        static_cast<unsigned>(
            std::clamp(
                iniInt(ini, "RuntimeProbeHeartbeatFrames", 600),
                60,
                36000
            )
        );

    return cfg;
}

} // namespace frr
