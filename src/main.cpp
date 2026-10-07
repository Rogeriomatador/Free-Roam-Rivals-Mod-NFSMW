#include <nfsmw_sdk/nfsmw_sdk.h>

#include "core/Config.h"
#include "core/Log.h"
#include "core/VersionGuard.h"
#include "game/RuntimeProbe.h"

#include <sstream>
#include <string>

namespace frr {

constexpr const char* kName = "NFSMW Free Roam Rivals";
constexpr const char* kVersion = "0.0.10-dev";

int bootstrap() {
    auto& log = Log::instance();

    log.info(std::string(kName) + " v" + kVersion + " loaded.");
    log.info("Running executable compatibility check.");

    const VersionCheck check =
        VersionGuard::checkCurrentExecutable();

    {
        std::ostringstream line;
        line << "Executable path: "
             << check.executable.path;
        log.info(line.str());
    }

    {
        std::ostringstream line;
        line << "Executable size: "
             << check.executable.size;
        log.info(line.str());
    }

    log.info(
        std::string("Executable MD5: ") +
        check.executable.md5
    );

    if (!check.supported) {
        log.warn(
            std::string("Unsupported executable: ") +
            check.reason
        );
        log.warn(
            "Fail-closed: runtime hooks, spawning, AI, economy and garage systems remain disabled."
        );
        return NFSMW_OK;
    }

    log.info(check.reason);
    log.info("Executable guard passed.");
    log.info(
        "Runtime stack: nfsmw-2005-sdk + MWSDK verified live registry + NFSPluginSDK typed gameplay structures."
    );

    const Config config = Config::load();

    game::RuntimeProbeConfig probeConfig{};
    probeConfig.renderProbeEnabled =
        config.renderProbeEnabled;
    probeConfig.inputProbeEnabled =
        config.inputProbeEnabled;
    probeConfig.roadNavDiagnosticsEnabled =
        config.roadNavDiagnosticsEnabled;
    probeConfig.frameTickProbeEnabled =
        config.frameTickProbeEnabled;
    probeConfig.sampleEveryFrames =
        config.runtimeSampleEveryFrames;
    probeConfig.heartbeatFrames =
        config.runtimeProbeHeartbeatFrames;
    probeConfig.experimentalSpawnEnabled =
        config.experimentalSpawnEnabled;
    probeConfig.stableFreeRoamSamplesBeforeSpawn =
        config.stableFreeRoamSamplesBeforeSpawn;
    probeConfig.maxActiveRivals =
        config.maxActiveRivals;
    probeConfig.undergroundBlacklistEnabled =
        config.undergroundBlacklistEnabled;
    probeConfig.undergroundBlacklistPersistence =
        config.undergroundBlacklistPersistence;

    const auto installed =
        game::RuntimeProbe::install(probeConfig);

    if (!installed.renderProbeArmed &&
        !installed.inputProbeInstalled) {
        log.error(
            "No runtime observation hook could be armed. All gameplay features remain disabled."
        );
    }

    log.info(
        "v0.0.10-dev adds live player road-navigation telemetry and mod-owned per-profile Underground Blacklist persistence using a pseudonymous profile key. Vehicle creation, AI mutation and frontend mutation remain disabled until their lifecycles are proven."
    );

    return NFSMW_OK;
}

} // namespace frr

NFSMW_PLUGIN_DECLARE(
    "Free Roam Rivals",
    "0.0.10-dev",
    "Rogeriomatador"
)

NFSMW_PLUGIN_MAIN() {
    return frr::bootstrap();
}
