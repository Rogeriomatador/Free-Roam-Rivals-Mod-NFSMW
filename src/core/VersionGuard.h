#pragma once

#include <cstdint>
#include <string>

namespace frr {

struct ExecutableIdentity {
    std::string path;
    std::string md5;
    std::uint64_t size = 0;
};

struct VersionCheck {
    ExecutableIdentity executable;
    bool supported = false;
    std::string reason;
};

class VersionGuard {
public:
    static VersionCheck checkCurrentExecutable();
};

} // namespace frr
