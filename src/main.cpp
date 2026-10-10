#include <nfsmw_sdk/nfsmw_sdk.h>

#include "core/Config.h"
#include "core/ExceptionTrace.h"
#include "core/Log.h"
#include "core/VersionGuard.h"
#include "game/RuntimeProbe.h"

#include <sstream>
#include <string>
#include <windows.h>
#include <filesystem>

namespace frr {

constexpr const char* kName = "NFSMW Free Roam Rivals";
constexpr const char* kVersion = "0.0.44-dev";

int bootstrap() {
    auto& log = Log::instance();

    log.info(std::string(kName) + " v" + kVersion + " loaded.");
    log.info("Running executable compatibility check.");

    const VersionCheck check =
        VersionGuard::checkCurrentExecutable();

    {
        std::ostringstream line;
        line << "Executable path: "
             << check.executable.path;
        log.info(line.str());
    }

    {
        std::ostringstream line;
        line << "Executable size: "
             << check.executable.size;
        log.info(line.str());
    }

    log.info(
        std::string("Executable MD5: ") +
        check.executable.md5
    );

    if (!check.supported) {
        log.warn(
            std::string("Unsupported executable: ") +
            check.reason
        );
        log.warn(
            "Fail-closed: runtime hooks, spawning, AI, economy and garage systems remain disabled."
        );
        return NFSMW_OK;
    }

    log.info(check.reason);
    log.info("Executable guard passed.");
    log.info(
        "Runtime stack: nfsmw-2005-sdk + MWSDK verified live registry + NFSPluginSDK typed gameplay structures."
    );

    const Config config = Config::load();
    if (!config.enabled) { log.info("General.Enabled=0: runtime hooks disabled."); return NFSMW_OK; }
    if(config.exceptionDiagnosticsEnabled) {
        wchar_t executable[32768]{};
        const auto count=GetModuleFileNameW(nullptr,executable,32768);
        bool traceInstalled=false;
        if(count>0 && count<32768) {
            const auto folder=std::filesystem::path(executable).parent_path()/L"scripts"/L"FreeRoamRivals";
            std::error_code error;
            std::filesystem::create_directories(folder,error);
            if(!error) traceInstalled=installExceptionTrace((folder/L"NativeExceptions.log").c_str(),kVersion);
        }
        if(traceInstalled) log.info("Native exception observer installed: scripts/FreeRoamRivals/NativeExceptions.log; first-chance only, context unchanged, continue-search; process-lifetime ASI.");
        else log.warn("Native exception observer unavailable; no startup fault address can be captured by this observer.");
    }
    ExceptionTracePhase bootstrapPhase("bootstrap_runtime_hooks");
    {
        std::ostringstream line;
        line << "Loaded diagnostics: render=" << config.renderProbeEnabled
             << " diagnosticBundle=" << config.diagnosticBundleEnabled
             << " input=" << config.inputProbeEnabled << " frameTick=" << config.frameTickProbeEnabled
             << " motionCapture=" << config.motionCaptureEnabled
             << " camera=" << config.cameraFrustumDiagnosticsEnabled
             << " worldCollision=" << config.worldCollisionDiagnosticsEnabled
             << " nativePrototype=" << config.nativeRivalPrototypeEnabled
             << " nativePrototypeNearPlayer=" << config.nativeRivalPrototypeNearPlayer
             << " sampleEveryFrames=" << config.runtimeSampleEveryFrames;
        log.info(line.str());
    }

    game::RuntimeProbeConfig probeConfig{};
    probeConfig.retireRivalVirtualKey=config.retireRivalVirtualKey;
    probeConfig.retireRequireControlShift=config.retireRequireControlShift;
    probeConfig.retireHoldSeconds=config.retireHoldSeconds;
    probeConfig.outrunEnabled=config.outrunEnabled;
    probeConfig.rivalHudEnabled=config.rivalHudEnabled;
    probeConfig.rivalHistoryEnabled=config.rivalHistoryEnabled;
    probeConfig.rivalChallengeDistanceMeters=config.rivalChallengeDistanceMeters;
    probeConfig.outrunWinLeadMeters=config.outrunWinLeadMeters;
    probeConfig.outrunLeadHoldSeconds=config.outrunLeadHoldSeconds;
    probeConfig.outrunMaxDurationSeconds=config.outrunMaxDurationSeconds;
    probeConfig.diagnosticBundleEnabled=config.diagnosticBundleEnabled;
    probeConfig.renderProbeEnabled =
        config.renderProbeEnabled;
    probeConfig.inputProbeEnabled =
        config.inputProbeEnabled;
    probeConfig.roadNavDiagnosticsEnabled =
        config.roadNavDiagnosticsEnabled;
    probeConfig.frameTickProbeEnabled =
        config.frameTickProbeEnabled;
    probeConfig.worldCollisionDiagnosticsEnabled =
        config.worldCollisionDiagnosticsEnabled;
    probeConfig.cameraFrustumDiagnosticsEnabled = config.cameraFrustumDiagnosticsEnabled;
    probeConfig.motionCaptureEnabled = config.motionCaptureEnabled;
    probeConfig.postRaceRacerDiagnosticsEnabled = config.postRaceRacerDiagnosticsEnabled;
    probeConfig.sampleEveryFrames =
        config.runtimeSampleEveryFrames;
    probeConfig.heartbeatFrames =
        config.runtimeProbeHeartbeatFrames;
    probeConfig.nativeRivalPrototypeEnabled = config.nativeRivalPrototypeEnabled;
    probeConfig.nativeRivalPrototypeNearPlayer = config.nativeRivalPrototypeNearPlayer;
    probeConfig.experimentalSpawnEnabled =
        config.experimentalSpawnEnabled;
    probeConfig.stableFreeRoamSamplesBeforeSpawn =
        config.stableFreeRoamSamplesBeforeSpawn;
    probeConfig.maxActiveRivals =
        config.maxActiveRivals;
    probeConfig.useHornToChallenge =
        config.useHornToChallenge;
    probeConfig.fallbackChallengeVirtualKey =
        config.fallbackChallengeVirtualKey;
    probeConfig.undergroundBlacklistEnabled =
        config.undergroundBlacklistEnabled;
    probeConfig.undergroundBlacklistPersistence =
        config.undergroundBlacklistPersistence;

    const auto installed =
        game::RuntimeProbe::install(probeConfig);

    if (!installed.renderProbeArmed &&
        !installed.inputProbeInstalled &&
        !installed.frameTickProbeInstalled &&
        !installed.gameplayLoopInstalled) {
        log.error(
            "No runtime observation hook could be armed. All gameplay features remain disabled."
        );
    }

    log.info(
        "v0.0.43-dev converts native fixed-millisecond frame ticks to callback seconds; keeps original stack word unchanged. Manual retirement defaults off; G refusals and race interruption reasons are recorded."
    );

    return NFSMW_OK;
}

} // namespace frr

NFSMW_PLUGIN_DECLARE(
    "Free Roam Rivals",
    "0.0.43-dev",
    "Rogeriomatador"
)

NFSMW_PLUGIN_MAIN() {
    return frr::bootstrap();
}
