#pragma once

namespace frr::game {

struct RuntimeProbeConfig {
    bool renderProbeEnabled = true;
    bool inputProbeEnabled = true;
    bool roadNavDiagnosticsEnabled = true;
    bool frameTickProbeEnabled = false;
    bool worldCollisionDiagnosticsEnabled = false;
    bool cameraFrustumDiagnosticsEnabled = false;
    unsigned sampleEveryFrames = 30;
    unsigned heartbeatFrames = 600;

    bool experimentalSpawnEnabled = false;
    unsigned stableFreeRoamSamplesBeforeSpawn = 6;
    int maxActiveRivals = 1;

    bool useHornToChallenge = true;
    unsigned fallbackChallengeVirtualKey = 0x47u;

    bool undergroundBlacklistEnabled = true;
    bool undergroundBlacklistPersistence = true;
};

struct RuntimeProbeInstallResult {
    bool renderProbeArmed = false;
    bool inputProbeInstalled = false;
    bool frameTickProbeInstalled = false;
};

class RuntimeProbe {
public:
    static RuntimeProbeInstallResult install(
        const RuntimeProbeConfig& config
    );
};

} // namespace frr::game

