#pragma once

namespace frr {

struct Config {
    bool runtimeProbeEnabled = true;
    unsigned runtimeProbeHeartbeatFrames = 600;

    static Config load();
};

} // namespace frr
