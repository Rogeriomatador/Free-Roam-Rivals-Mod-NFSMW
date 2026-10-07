#include "Config.h"

#include <windows.h>

#include <algorithm>
#include <cstdlib>
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
    const char* section,
    const char* key,
    int fallback
) {
    return GetPrivateProfileIntA(
        section,
        key,
        fallback,
        file.c_str()
    );
}

unsigned iniUnsigned(
    const std::string& file,
    const char* section,
    const char* key,
    unsigned fallback
) {
    char buffer[64]{};
    const std::string fallbackText =
        std::to_string(fallback);

    GetPrivateProfileStringA(
        section,
        key,
        fallbackText.c_str(),
        buffer,
        static_cast<DWORD>(sizeof(buffer)),
        file.c_str()
    );

    char* end = nullptr;
    const unsigned long value =
        std::strtoul(buffer, &end, 0);

    if (end == buffer) {
        return fallback;
    }

    while (*end == ' ' || *end == '\t') {
        ++end;
    }

    if (*end != '\0') {
        return fallback;
    }

    return static_cast<unsigned>(value);
}

float iniFloat(
    const std::string& file,
    const char* section,
    const char* key,
    float fallback
) {
    char buffer[64]{};
    const std::string fallbackText =
        std::to_string(fallback);

    GetPrivateProfileStringA(
        section,
        key,
        fallbackText.c_str(),
        buffer,
        static_cast<DWORD>(sizeof(buffer)),
        file.c_str()
    );

    char* end = nullptr;
    const float value = std::strtof(buffer, &end);
    if (end == buffer) {
        return fallback;
    }

    return value;
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
        iniInt(
            ini,
            "Diagnostics",
            "RenderProbeEnabled",
            1
        ) != 0;

    cfg.inputProbeEnabled =
        iniInt(
            ini,
            "Diagnostics",
            "InputProbeEnabled",
            1
        ) != 0;

    cfg.roadNavDiagnosticsEnabled =
        iniInt(
            ini,
            "Diagnostics",
            "RoadNavDiagnosticsEnabled",
            1
        ) != 0;

    cfg.frameTickProbeEnabled =
        iniInt(
            ini,
            "Diagnostics",
            "FrameTickProbeEnabled",
            0
        ) != 0;

    cfg.worldCollisionDiagnosticsEnabled =
        iniInt(
            ini,
            "Diagnostics",
            "WorldCollisionDiagnosticsEnabled",
            0
        ) != 0;

    cfg.cameraFrustumDiagnosticsEnabled =
        iniInt(ini, "Diagnostics", "CameraFrustumDiagnosticsEnabled", 0) != 0;
    cfg.motionCaptureEnabled =
        iniInt(ini, "Diagnostics", "MotionCaptureEnabled", 0) != 0;

    cfg.runtimeSampleEveryFrames =
        static_cast<unsigned>(
            std::clamp(
                iniInt(
                    ini,
                    "Diagnostics",
                    "RuntimeSampleEveryFrames",
                    30
                ),
                1,
                600
            )
        );

    cfg.runtimeProbeHeartbeatFrames =
        static_cast<unsigned>(
            std::clamp(
                iniInt(
                    ini,
                    "Diagnostics",
                    "RuntimeProbeHeartbeatFrames",
                    600
                ),
                60,
                36000
            )
        );

    cfg.experimentalSpawnEnabled =
        iniInt(
            ini,
            "Experimental",
            "ExperimentalSpawnEnabled",
            0
        ) != 0;

    cfg.experimentalAIControlEnabled =
        iniInt(
            ini,
            "Experimental",
            "ExperimentalAIControlEnabled",
            0
        ) != 0;

    cfg.stableFreeRoamSamplesBeforeSpawn =
        static_cast<unsigned>(
            std::clamp(
                iniInt(
                    ini,
                    "Experimental",
                    "StableFreeRoamSamplesBeforeSpawn",
                    6
                ),
                1,
                120
            )
        );

    cfg.maxActiveRivals = std::clamp(
        iniInt(ini, "Rivals", "MaxActiveRivals", 1),
        0,
        8
    );

    cfg.spawnMinDistanceMeters = std::clamp(
        iniFloat(
            ini,
            "Rivals",
            "SpawnMinDistanceMeters",
            350.0f
        ),
        50.0f,
        5000.0f
    );

    cfg.spawnMaxDistanceMeters = std::clamp(
        iniFloat(
            ini,
            "Rivals",
            "SpawnMaxDistanceMeters",
            850.0f
        ),
        cfg.spawnMinDistanceMeters,
        10000.0f
    );

    cfg.noSpawnVisibleRadiusMeters = std::clamp(
        iniFloat(
            ini,
            "Rivals",
            "NoSpawnVisibleRadiusMeters",
            300.0f
        ),
        0.0f,
        cfg.spawnMaxDistanceMeters
    );

    cfg.despawnDistanceMeters = std::clamp(
        iniFloat(
            ini,
            "Rivals",
            "DespawnDistanceMeters",
            1400.0f
        ),
        cfg.spawnMaxDistanceMeters,
        20000.0f
    );

    cfg.interestRadiusMeters = std::clamp(
        iniFloat(
            ini,
            "Rivals",
            "InterestRadiusMeters",
            90.0f
        ),
        1.0f,
        1000.0f
    );

    cfg.challengeRadiusMeters = std::clamp(
        iniFloat(
            ini,
            "Rivals",
            "ChallengeRadiusMeters",
            20.0f
        ),
        1.0f,
        cfg.interestRadiusMeters
    );

    cfg.challengeTimeoutSeconds = std::clamp(
        iniFloat(
            ini,
            "Rivals",
            "ChallengeTimeoutSeconds",
            12.0f
        ),
        1.0f,
        120.0f
    );

    cfg.useHornToChallenge =
        iniInt(
            ini,
            "Input",
            "UseHornToChallenge",
            1
        ) != 0;

    cfg.fallbackChallengeVirtualKey =
        std::clamp(
            iniUnsigned(
                ini,
                "Input",
                "FallbackChallengeKey",
                0x47u
            ),
            0u,
            0xFEu
        );

    cfg.outrunEnabled =
        iniInt(ini, "Outrun", "Enabled", 0) != 0;

    cfg.outrunWinLeadMeters = std::clamp(
        iniFloat(
            ini,
            "Outrun",
            "WinLeadMeters",
            300.0f
        ),
        10.0f,
        5000.0f
    );

    cfg.outrunLeadHoldSeconds = std::clamp(
        iniFloat(
            ini,
            "Outrun",
            "LeadHoldSeconds",
            3.0f
        ),
        0.0f,
        60.0f
    );

    cfg.outrunMaxDurationSeconds = std::clamp(
        iniFloat(
            ini,
            "Outrun",
            "MaxDurationSeconds",
            300.0f
        ),
        10.0f,
        3600.0f
    );

    cfg.stagingEnabled =
        iniInt(ini, "Staging", "Enabled", 0) != 0;

    cfg.stagingSearchAheadMeters = std::clamp(
        iniFloat(
            ini,
            "Staging",
            "SearchAheadMeters",
            160.0f
        ),
        20.0f,
        1000.0f
    );

    cfg.stagingApproachTimeoutSeconds = std::clamp(
        iniFloat(
            ini,
            "Staging",
            "ApproachTimeoutSeconds",
            10.0f
        ),
        1.0f,
        120.0f
    );

    cfg.stagingAlignmentTimeoutSeconds = std::clamp(
        iniFloat(
            ini,
            "Staging",
            "AlignmentTimeoutSeconds",
            5.0f
        ),
        1.0f,
        60.0f
    );

    cfg.stagingHiddenAlignmentFallback =
        iniInt(
            ini,
            "Staging",
            "AllowHiddenAlignmentFallback",
            1
        ) != 0;

    cfg.undergroundBlacklistEnabled =
        iniInt(
            ini,
            "UndergroundBlacklist",
            "Enabled",
            1
        ) != 0;

    cfg.undergroundBlacklistPersistence =
        iniInt(
            ini,
            "UndergroundBlacklist",
            "PersistProgress",
            1
        ) != 0;

    return cfg;
}

} // namespace frr
