#include <nfsmw_sdk/nfsmw_sdk.h>

#include "core/Log.h"
#include "core/VersionGuard.h"

#include <sstream>
#include <string>

namespace frr {

constexpr const char* kName = "NFSMW Free Roam Rivals";
constexpr const char* kVersion = "0.0.2-dev";

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
        log.warn("Fail-closed: no gameplay hooks or save/economy features will be enabled.");
        return NFSMW_OK;
    }

    log.info(check.reason);
    log.info("Executable guard passed.");
    log.info("Gameplay hooks are still disabled in this development build.");
    log.info("Economy / garage / pink-slip systems remain disabled.");

    return NFSMW_OK;
}

} // namespace frr

NFSMW_PLUGIN_DECLARE("Free Roam Rivals", "0.0.2-dev", "Rogeriomatador")

NFSMW_PLUGIN_MAIN() {
    return frr::bootstrap();
}
