#pragma once

namespace frr::game {

struct RuntimeProbeConfig {
    bool renderProbeEnabled = true;
    bool inputProbeEnabled = true;
    unsigned sampleEveryFrames = 30;
    unsigned heartbeatFrames = 600;
};

struct RuntimeProbeInstallResult {
    bool renderProbeArmed = false;
    bool inputProbeInstalled = false;
};

class RuntimeProbe {
public:
    static RuntimeProbeInstallResult install(
        const RuntimeProbeConfig& config
    );
};

} // namespace frr::game
