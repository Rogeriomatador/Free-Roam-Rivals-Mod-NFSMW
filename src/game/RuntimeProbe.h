#pragma once
#include <string>

namespace frr::game {

struct RuntimeProbeConfig {
    // Populated ONLY after the exact executable passes VersionGuard.
    std::string verifiedExecutableMd5;
    bool diagnosticBundleEnabled = false;
    bool renderProbeEnabled = true;
    bool inputProbeEnabled = true;
    bool roadNavDiagnosticsEnabled = true;
    bool frameTickProbeEnabled = false;
    bool worldCollisionDiagnosticsEnabled = false;
    bool cameraFrustumDiagnosticsEnabled = false;
    bool motionCaptureEnabled = false;
    bool postRaceRacerDiagnosticsEnabled = false;
    unsigned sampleEveryFrames = 30;
    unsigned heartbeatFrames = 600;

    bool outrunEnabled=false, rivalHudEnabled=true, rivalHistoryEnabled=true;
    bool nativeStreetDuelPilotEnabled=false;
    float streetSprintLengthMeters=120.0f;
    int rivalRadarPosition=2;
    float rivalRadarScale=1.0f;
    float rivalChallengeDistanceMeters=60, outrunWinLeadMeters=300, outrunLeadHoldSeconds=3, outrunMaxDurationSeconds=300;
    unsigned retireRivalVirtualKey=0;
    bool retireRequireControlShift=true;
    float retireHoldSeconds=1.5f;
    bool nativeRivalPrototypeEnabled = false;
    bool nativeRivalPrototypeNearPlayer = false;
    bool experimentalSpawnEnabled = false;
    unsigned stableFreeRoamSamplesBeforeSpawn = 6;
    int maxActiveRivals = 1;

    bool useHornToChallenge = true;
    unsigned fallbackChallengeVirtualKey = 0x47u;

    bool undergroundBlacklistEnabled = true;
    bool undergroundBlacklistPersistence = true;
};

struct RuntimeProbeInstallResult {
    bool gameplayLoopInstalled = false;
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
