#pragma once

#include <cstdint>

namespace frr::domain {

struct MotionScaleTuning {
    float minimumDeltaSeconds = 0.10f;
    float maximumDeltaSeconds = 2.50f;
    float minimumEngineSpeed = 2.0f;
    float maximumRelativeSpeedChange = 0.25f;
    float minimumWorldDistance = 0.05f;
    unsigned minimumStableSamples = 12;
    float maximumCoefficientOfVariation = 0.05f;
};

struct MotionScaleFrame {
    bool valid = false;
    bool safeFreeRoam = false;
    bool grounded = false;

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    // Unit semantics are intentionally NOT assumed here.
    float engineSpeed = 0.0f;
    float speedometer = 0.0f;
    float absoluteSpeed = 0.0f;
    float localVelocityMagnitude = 0.0f;
    float linearVelocityMagnitude = 0.0f;
};

struct MotionScaleSnapshot {
    unsigned acceptedSamples = 0;
    unsigned rejectedSamples = 0;

    // world units / (engine speed unit * second)
    float meanWorldUnitsPerSpeedUnitSecond = 0.0f;
    float coefficientOfVariation = 0.0f;

    float meanSpeedometerToEngineSpeedRatio = 0.0f;
    float meanAbsoluteToEngineSpeedRatio = 0.0f;
    float meanSpeedToLocalVelocityRatio = 0.0f;
    float meanSpeedToLinearVelocityRatio = 0.0f;

    bool stable = false;
};

class MotionScaleObserver {
public:
    explicit MotionScaleObserver(
        MotionScaleTuning tuning = {}
    );

    void reset();

    // deltaSeconds is observational elapsed time supplied by the runtime.
    // A stable result is evidence about consistency only. It does not prove
    // that an engine speed unit is one metre per second.
    MotionScaleSnapshot push(
        const MotionScaleFrame& frame,
        float deltaSeconds
    );

    MotionScaleSnapshot snapshot() const;

private:
    bool acceptPair(
        const MotionScaleFrame& previous,
        const MotionScaleFrame& current,
        float deltaSeconds,
        float& ratio,
        float& speedometerToSpeed,
        float& absoluteToSpeed,
        float& speedToLocal,
        float& speedToLinear
    ) const;

    MotionScaleTuning tuning_{};

    bool havePrevious_ = false;
    MotionScaleFrame previous_{};

    unsigned acceptedSamples_ = 0;
    unsigned rejectedSamples_ = 0;

    double ratioSum_ = 0.0;
    double ratioSquareSum_ = 0.0;
    double speedometerToSpeedSum_ = 0.0;
    double absoluteToSpeedSum_ = 0.0;
    double speedToLocalSum_ = 0.0;
    double speedToLinearSum_ = 0.0;
};

} // namespace frr::domain
