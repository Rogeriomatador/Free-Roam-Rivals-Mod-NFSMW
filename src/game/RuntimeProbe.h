#pragma once

namespace frr::game {

struct RuntimeProbeConfig {
    unsigned heartbeatFrames = 600;
};

class RuntimeProbe {
public:
    static bool install(const RuntimeProbeConfig& config);
};

} // namespace frr::game
