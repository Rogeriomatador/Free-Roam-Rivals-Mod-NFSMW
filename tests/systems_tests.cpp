#include "domain/OutrunRace.h"
#include "domain/RuntimeSession.h"
#include "domain/RivalRuntimeHandle.h"
#include "domain/SpawnSafety.h"
#include "domain/StagingPlanner.h"
#include "domain/StagingStateMachine.h"

#include <cstdlib>
#include <iostream>
#include <vector>

namespace {

void require(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace frr::domain;

    RuntimeSessionTracker sessions;
    RuntimeSessionObservation obs{};
    obs.safeFreeRoam = true;
    obs.playerIdentity = 0x1000;
    obs.roadNetworkIdentity = 0x2000;

    auto session = sessions.tick(obs);
    require(
        session.newGeneration &&
        session.generation == 1 &&
        session.stableSamples == 1,
        "session begins generation"
    );

    session = sessions.tick(obs);
    require(
        !session.newGeneration &&
        session.stableSamples == 2,
        "session stability increments"
    );

    obs.safeFreeRoam = false;
    session = sessions.tick(obs);
    require(
        !session.active &&
        session.stableSamples == 0,
        "unsafe transition invalidates session"
    );

    obs.safeFreeRoam = true;
    session = sessions.tick(obs);
    require(
        session.newGeneration &&
        session.generation == 2,
        "re-entry creates generation"
    );

    RivalRuntimeHandle handle;
    RivalRuntimePointers runtimePointers{};
    runtimePointers.iVehicle = 0x3000;
    runtimePointers.pVehicle = 0x4000;
    runtimePointers.vehicleAI = 0x5000;

    handle.bind(
        42,
        session.generation,
        runtimePointers,
        true
    );

    require(
        handle.validForGeneration(session.generation),
        "runtime handle is scoped to world generation"
    );
    require(
        !handle.validForGeneration(session.generation + 1),
        "stale generation invalidates runtime handle"
    );

    handle.markDestroyPending();
    require(
        !handle.validForGeneration(session.generation) &&
        handle.destroyPending(),
        "destroy-pending handle cannot be used"
    );

    handle.invalidate();
    require(
        !handle.bound(),
        "runtime handle clears all live pointers"
    );

    SpawnEnvironmentInput spawnEnv{};
    spawnEnv.experimentalFeatureEnabled = true;
    spawnEnv.supportedExecutable = true;
    spawnEnv.freeRoamCandidate = true;
    spawnEnv.playerAvailable = true;
    spawnEnv.independentPlayerCrossCheck = true;
    spawnEnv.roadNetworkAvailable = true;
    spawnEnv.stableFreeRoamSamples = 6;
    spawnEnv.liveRivals = 0;
    spawnEnv.maxLiveRivals = 1;

    SpawnCandidateInput spawnCandidate{};
    spawnCandidate.available = true;
    spawnCandidate.vehicleAvailable = true;
    spawnCandidate.roadValid = true;
    spawnCandidate.groundValid = true;
    spawnCandidate.visibleToPlayer = false;
    spawnCandidate.distanceFromPlayerMeters = 500.0f;

    auto spawn =
        evaluateSpawnCandidate(spawnEnv, spawnCandidate);

    require(
        spawn.allowed,
        "safe hidden candidate allowed"
    );

    spawnCandidate.visibleToPlayer = true;
    spawn = evaluateSpawnCandidate(spawnEnv, spawnCandidate);
    require(
        !spawn.allowed &&
        spawn.reason == SpawnRejectReason::VisiblePopInRisk,
        "visible spawn blocked"
    );

    spawnCandidate.visibleToPlayer = false;
    spawnCandidate.distanceFromPlayerMeters = 900.0f;
    spawn = evaluateSpawnCandidate(spawnEnv, spawnCandidate);
    require(
        !spawn.allowed &&
        spawn.reason ==
            SpawnRejectReason::TooFarWithoutStreamingProof,
        "far spawn needs streaming proof"
    );

    spawnCandidate.streamingVerified = true;
    spawn = evaluateSpawnCandidate(spawnEnv, spawnCandidate);
    require(
        spawn.allowed,
        "far spawn allowed only with streaming proof"
    );

    OutrunTuning outrunTuning{};
    outrunTuning.winLeadMeters = 100.0f;
    outrunTuning.leadHoldSeconds = 2.0f;
    outrunTuning.maxDurationSeconds = 30.0f;

    OutrunRace outrun(outrunTuning);
    outrun.begin();

    OutrunInput raceInput{};
    raceInput.signedLeadMeters = 110.0f;
    raceInput.deltaSeconds = 1.0f;

    require(
        outrun.tick(raceInput) ==
            OutrunOutcome::InProgress,
        "outrun lead starts hold"
    );
    require(
        outrun.holdProgress01() > 0.49f,
        "outrun hold progress exposed"
    );
    require(
        outrun.tick(raceInput) ==
            OutrunOutcome::PlayerWon,
        "outrun player wins after held lead"
    );

    outrun.begin();
    raceInput.signedLeadMeters = -110.0f;
    raceInput.deltaSeconds = 1.0f;
    require(
        outrun.tick(raceInput) ==
            OutrunOutcome::InProgress,
        "rival lead starts hold"
    );

    raceInput.signedLeadMeters = 0.0f;
    require(
        outrun.tick(raceInput) ==
            OutrunOutcome::InProgress &&
        outrun.holdSeconds() == 0.0f,
        "lead loss resets hold"
    );

    StagingCandidate bad{};
    bad.roadValid = true;
    bad.streamed = true;
    bad.groundValid = true;
    bad.supportsTwoCars = true;
    bad.distanceAheadMeters = 100.0f;
    bad.roadWidthMeters = 8.0f;
    bad.junction = true;

    StagingCandidate good = bad;
    good.junction = false;
    good.distanceAheadMeters = 120.0f;
    good.roadWidthMeters = 9.0f;
    good.absoluteCurvature = 0.005f;
    good.absoluteGrade = 0.02f;

    StagingCandidate acceptable = good;
    acceptable.distanceAheadMeters = 190.0f;
    acceptable.roadWidthMeters = 7.2f;
    acceptable.absoluteCurvature = 0.015f;

    const std::vector<StagingCandidate> candidates = {
        bad,
        acceptable,
        good
    };

    const auto best =
        selectBestStagingCandidate(candidates);

    require(
        best &&
        *best == 2,
        "staging planner chooses safest straight candidate"
    );

    StagingTuning stagingTuning{};
    stagingTuning.alignmentTimeoutSeconds = 1.0f;

    StagingStateMachine staging(stagingTuning);
    StagingInput stagingInput{};
    stagingInput.stagingCandidateFound = true;
    staging.tick(stagingInput);

    require(
        staging.state() == StagingState::Reserve,
        "staging found candidate"
    );

    stagingInput.reservationValid = true;
    staging.tick(stagingInput);
    require(
        staging.state() == StagingState::Approach,
        "staging reserved candidate"
    );

    stagingInput.approachComplete = true;
    staging.tick(stagingInput);
    require(
        staging.state() == StagingState::Align,
        "staging enters alignment"
    );

    stagingInput.hiddenAlignmentTransformSafe = true;
    stagingInput.deltaSeconds = 1.1f;
    auto stagingUpdate = staging.tick(stagingInput);

    require(
        staging.state() == StagingState::CameraIntro &&
        stagingUpdate.requestHiddenAlignment,
        "alignment timeout requests hidden safe correction"
    );

    stagingInput.cameraIntroComplete = true;
    stagingUpdate = staging.tick(stagingInput);
    require(
        staging.state() == StagingState::Negotiating &&
        stagingUpdate.cinematicCameraActive,
        "camera intro hands off to negotiation"
    );

    stagingInput.stakeConfirmed = true;
    staging.tick(stagingInput);
    stagingInput.enginesReady = true;
    staging.tick(stagingInput);
    stagingInput.countdownComplete = true;
    staging.tick(stagingInput);
    stagingInput.releaseComplete = true;
    stagingUpdate = staging.tick(stagingInput);

    require(
        stagingUpdate.completed &&
        stagingUpdate.restorePlayerControl &&
        stagingUpdate.restoreCamera,
        "staging completion restores camera and controls"
    );

    staging.reset();
    stagingInput = {};
    stagingInput.worldSafe = false;
    stagingUpdate = staging.tick(stagingInput);

    require(
        stagingUpdate.aborted &&
        stagingUpdate.restorePlayerControl &&
        stagingUpdate.restoreCamera,
        "unsafe transition aborts staging with cleanup directives"
    );

    std::cout
        << "Free Roam Rivals systems tests passed.\n";
    return 0;
}
