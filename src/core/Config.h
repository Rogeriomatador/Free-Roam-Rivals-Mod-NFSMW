#pragma once

namespace frr {

struct Config {
    bool renderProbeEnabled = true;
    bool inputProbeEnabled = true;
    unsigned runtimeSampleEveryFrames = 30;
    unsigned runtimeProbeHeartbeatFrames = 600;

    static Config load();
};

} // namespace frr
