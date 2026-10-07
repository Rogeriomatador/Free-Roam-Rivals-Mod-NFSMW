#pragma once

namespace frr {

struct Config {
    bool renderProbeEnabled = true;
    bool inputProbeEnabled = true;
    bool roadNavDiagnosticsEnabled = true;
    bool frameTickProbeEnabled = false;
    bool worldCollisionDiagnosticsEnabled = false;
    bool cameraFrustumDiagnosticsEnabled = false;
    bool motionCaptureEnabled = false;
    unsigned runtimeSampleEveryFrames = 30;
    unsigned runtimeProbeHeartbeatFrames = 600;

    bool experimentalSpawnEnabled = false;
    bool experimentalAIControlEnabled = false;
    unsigned stableFreeRoamSamplesBeforeSpawn = 6;

    int maxActiveRivals = 1;
    float spawnMinDistanceMeters = 350.0f;
    float spawnMaxDistanceMeters = 850.0f;
    float noSpawnVisibleRadiusMeters = 300.0f;
    float despawnDistanceMeters = 1400.0f;
    float interestRadiusMeters = 90.0f;
    float challengeRadiusMeters = 20.0f;
    float challengeTimeoutSeconds = 12.0f;

    bool useHornToChallenge = true;
    unsigned fallbackChallengeVirtualKey = 0x47u;

    bool outrunEnabled = false;
    float outrunWinLeadMeters = 300.0f;
    float outrunLeadHoldSeconds = 3.0f;
    float outrunMaxDurationSeconds = 300.0f;

    bool stagingEnabled = false;
    float stagingSearchAheadMeters = 160.0f;
    float stagingApproachTimeoutSeconds = 10.0f;
    float stagingAlignmentTimeoutSeconds = 5.0f;
    bool stagingHiddenAlignmentFallback = true;

    bool undergroundBlacklistEnabled = true;
    bool undergroundBlacklistPersistence = true;

    static Config load();
};

} // namespace frr
