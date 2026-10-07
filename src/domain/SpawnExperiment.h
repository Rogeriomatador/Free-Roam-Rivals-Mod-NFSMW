#pragma once

namespace frr::domain {

enum class SpawnExperimentState {
    Idle,
    AwaitConstruction,
    VerifyRegistries,
    VerifyAI,
    ObserveMotion,
    Cleanup,
    VerifyRemoval,
    Succeeded,
    Failed
};

enum class SpawnExperimentFailure {
    None,
    PreconditionsLost,
    GenerationChanged,
    ConstructionFailed,
    ConstructionTimeout,
    RegistryTimeout,
    AiTimeout,
    MotionTimeout,
    CleanupTimeout
};

struct SpawnExperimentTuning {
    float constructionTimeoutSeconds = 1.5f;
    float registryTimeoutSeconds = 2.0f;
    float aiTimeoutSeconds = 2.0f;
    float requiredMotionObservationSeconds = 3.0f;
    float motionTimeoutSeconds = 8.0f;
    float cleanupTimeoutSeconds = 4.0f;
    unsigned removalConfirmSamples = 2;
};

struct SpawnExperimentInput {
    float deltaSeconds = 0.0f;

    bool worldSafe = true;
    bool generationValid = true;
    bool candidateValid = true;

    bool constructionSucceeded = false;
    bool constructionFailed = false;

    bool pvehicleRegistered = false;
    bool liveVehicleRegistered = false;
    bool aiAvailable = false;
    bool nativeMotionObserved = false;
};

struct SpawnExperimentUpdate {
    SpawnExperimentState state = SpawnExperimentState::Idle;
    SpawnExperimentFailure failure = SpawnExperimentFailure::None;

    bool stateChanged = false;
    bool requestConstruct = false;
    bool requestCleanup = false;

    bool ownsVehicle = false;
    bool lifecycleProven = false;
    bool disableSpawningForSession = false;
};

class SpawnExperiment {
public:
    explicit SpawnExperiment(
        SpawnExperimentTuning tuning = {}
    );

    void begin();
    void reset();

    SpawnExperimentUpdate tick(
        const SpawnExperimentInput& input
    );

    SpawnExperimentState state() const;
    SpawnExperimentFailure failure() const;
    bool ownsVehicle() const;
    float stateTimeSeconds() const;

private:
    void transition(SpawnExperimentState next);
    void requestFailure(SpawnExperimentFailure reason);
    void finishRemoval();

    SpawnExperimentUpdate makeUpdate(
        SpawnExperimentState before,
        bool requestConstruct,
        bool requestCleanup
    ) const;

    SpawnExperimentTuning tuning_{};
    SpawnExperimentState state_ =
        SpawnExperimentState::Idle;
    SpawnExperimentFailure failure_ =
        SpawnExperimentFailure::None;
    SpawnExperimentFailure pendingFailure_ =
        SpawnExperimentFailure::None;

    float stateTimeSeconds_ = 0.0f;
    bool ownsVehicle_ = false;
    bool constructRequested_ = false;
    bool cleanupRequested_ = false;
    bool motionObserved_ = false;
    unsigned removalConfirmSamples_ = 0;
};

const char* spawnExperimentStateName(
    SpawnExperimentState state
);

const char* spawnExperimentFailureName(
    SpawnExperimentFailure failure
);

} // namespace frr::domain
