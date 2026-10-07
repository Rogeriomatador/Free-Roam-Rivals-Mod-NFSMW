#include "domain/SpawnExperiment.h"

#include <cstdlib>
#include <iostream>

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

    SpawnExperimentTuning tuning{};
    tuning.constructionTimeoutSeconds = 1.0f;
    tuning.registryTimeoutSeconds = 1.0f;
    tuning.aiTimeoutSeconds = 1.0f;
    tuning.requiredMotionObservationSeconds = 2.0f;
    tuning.motionTimeoutSeconds = 4.0f;
    tuning.cleanupTimeoutSeconds = 2.0f;
    tuning.removalConfirmSamples = 2;

    SpawnExperiment experiment(tuning);
    SpawnExperimentInput input{};
    input.deltaSeconds = 0.1f;

    experiment.begin();

    auto update = experiment.tick(input);
    require(
        update.requestConstruct &&
        update.state ==
            SpawnExperimentState::AwaitConstruction,
        "experiment requests construction exactly after begin"
    );

    update = experiment.tick(input);
    require(
        !update.requestConstruct,
        "construction command is not repeated"
    );

    input.constructionSucceeded = true;
    update = experiment.tick(input);
    require(
        update.state ==
            SpawnExperimentState::VerifyRegistries &&
        update.ownsVehicle,
        "constructed vehicle enters registry verification"
    );

    input.constructionSucceeded = false;
    input.pvehicleRegistered = true;
    input.liveVehicleRegistered = true;
    update = experiment.tick(input);
    require(
        update.state == SpawnExperimentState::VerifyAI,
        "both registries are required before AI verification"
    );

    input.aiAvailable = true;
    update = experiment.tick(input);
    require(
        update.state ==
            SpawnExperimentState::ObserveMotion,
        "AI availability enters native-motion observation"
    );

    input.aiAvailable = false;
    input.nativeMotionObserved = true;
    input.deltaSeconds = 1.0f;

    update = experiment.tick(input);
    require(
        update.state ==
            SpawnExperimentState::ObserveMotion,
        "motion must remain observed for minimum window"
    );

    update = experiment.tick(input);
    require(
        update.state == SpawnExperimentState::Cleanup,
        "successful observation always enters cleanup"
    );

    input.nativeMotionObserved = false;
    input.deltaSeconds = 0.1f;
    update = experiment.tick(input);
    require(
        update.requestCleanup &&
        update.state ==
            SpawnExperimentState::VerifyRemoval,
        "cleanup command is explicit and one-shot"
    );

    input.pvehicleRegistered = false;
    input.liveVehicleRegistered = false;

    update = experiment.tick(input);
    require(
        update.state ==
            SpawnExperimentState::VerifyRemoval,
        "removal needs repeated confirmation"
    );

    update = experiment.tick(input);
    require(
        update.lifecycleProven &&
        update.state ==
            SpawnExperimentState::Succeeded &&
        !update.ownsVehicle,
        "full create-drive-cleanup lifecycle succeeds only after removal proof"
    );

    experiment.begin();
    input = {};
    input.deltaSeconds = 0.1f;
    input.worldSafe = false;

    update = experiment.tick(input);
    require(
        update.state == SpawnExperimentState::Failed &&
        update.failure ==
            SpawnExperimentFailure::PreconditionsLost &&
        !update.requestCleanup &&
        update.disableSpawningForSession,
        "unsafe world before construction fails without cleanup"
    );

    experiment.begin();
    input = {};
    input.deltaSeconds = 0.1f;
    experiment.tick(input);

    input.constructionFailed = true;
    update = experiment.tick(input);
    require(
        update.state == SpawnExperimentState::Failed &&
        update.failure ==
            SpawnExperimentFailure::ConstructionFailed,
        "explicit construction failure disables the session experiment"
    );

    experiment.begin();
    input = {};
    input.deltaSeconds = 0.1f;
    experiment.tick(input);

    input.constructionSucceeded = true;
    experiment.tick(input);

    input.constructionSucceeded = false;
    input.deltaSeconds = 1.1f;
    update = experiment.tick(input);

    require(
        update.state == SpawnExperimentState::Cleanup,
        "registry timeout routes owned vehicle through cleanup"
    );

    input.deltaSeconds = 0.1f;
    update = experiment.tick(input);
    require(
        update.requestCleanup &&
        update.state ==
            SpawnExperimentState::VerifyRemoval,
        "failed lifecycle still issues cleanup exactly once"
    );

    input.pvehicleRegistered = false;
    input.liveVehicleRegistered = false;
    experiment.tick(input);
    update = experiment.tick(input);

    require(
        update.state == SpawnExperimentState::Failed &&
        update.failure ==
            SpawnExperimentFailure::RegistryTimeout &&
        update.disableSpawningForSession,
        "original failure is preserved after successful cleanup"
    );

    experiment.begin();
    input = {};
    input.deltaSeconds = 0.1f;
    experiment.tick(input);

    input.constructionSucceeded = true;
    experiment.tick(input);

    input.constructionSucceeded = false;
    input.pvehicleRegistered = true;
    input.liveVehicleRegistered = true;
    experiment.tick(input);

    input.worldSafe = false;
    update = experiment.tick(input);
    require(
        update.state == SpawnExperimentState::Cleanup,
        "unsafe transition after ownership requests cleanup path"
    );

    input.worldSafe = true;
    update = experiment.tick(input);
    require(
        update.requestCleanup,
        "cleanup is requested after post-construction unsafe transition"
    );

    input.pvehicleRegistered = false;
    input.liveVehicleRegistered = false;
    experiment.tick(input);
    update = experiment.tick(input);

    require(
        update.state == SpawnExperimentState::Failed &&
        update.failure ==
            SpawnExperimentFailure::PreconditionsLost,
        "post-construction unsafe transition fails only after cleanup proof"
    );

    std::cout
        << "Free Roam Rivals spawn experiment tests passed.\n";
    return 0;
}
