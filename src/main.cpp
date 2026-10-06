#include <nfsmw_sdk/nfsmw_sdk.h>

#include "core/Config.h"
#include "core/Log.h"
#include "core/VersionGuard.h"
#include "game/RuntimeProbe.h"

#include <sstream>
#include <string>

namespace frr {

constexpr const char* kName = "NFSMW Free Roam Rivals";
constexpr const char* kVersion = "0.0.4-dev";

int bootstrap() {
    auto& log = Log::instance();

    log.info(std::string(kName) + " v" + kVersion + " loaded.");
    log.info("Running executable compatibility check.");

    const VersionCheck check = VersionGuard::checkCurrentExecutable();

    {
        std::ostringstream line;
        line << "Executable path: " << check.executable.path;
        log.info(line.str());
    }

    {
        std::ostringstream line;
        line << "Executable size: " << check.executable.size;
        log.info(line.str());
    }

    log.info(std::string("Executable MD5: ") + check.executable.md5);

    if (!check.supported) {
        log.warn(std::string("Unsupported executable: ") + check.reason);
        log.warn("Fail-closed: no runtime hooks or save/economy features will be enabled.");
        return NFSMW_OK;
    }

    log.info(check.reason);
    log.info("Executable guard passed.");

    const Config config = Config::load();

    if (config.runtimeProbeEnabled) {
        game::RuntimeProbeConfig probeConfig{};
        probeConfig.heartbeatFrames =
            config.runtimeProbeHeartbeatFrames;

        if (!game::RuntimeProbe::install(probeConfig)) {
            log.error("Runtime probe could not be installed; continuing with all gameplay features disabled.");
        }
    } else {
        log.info("Runtime probe disabled by configuration.");
    }

    log.info("v0.0.4-dev is read-only: no vehicle spawning, AI control, economy, garage, or pink-slip mutation is enabled.");

    return NFSMW_OK;
}

} // namespace frr

NFSMW_PLUGIN_DECLARE("Free Roam Rivals", "0.0.4-dev", "Rogeriomatador")

NFSMW_PLUGIN_MAIN() {
    return frr::bootstrap();
}
