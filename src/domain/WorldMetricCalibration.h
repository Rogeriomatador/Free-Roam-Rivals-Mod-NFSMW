#pragma once

#include "MotionScaleObserver.h"

#include <optional>

namespace frr::domain {

struct WorldMetricCalibration {
    bool verified = false;
    float worldUnitsPerMeter = 0.0f;
};

struct MotionMetricCalibrationTuning {
    unsigned minimumStableSamples = 20;
    float maximumCoefficientOfVariation = 0.02f;
    float minimumAbsoluteToEngineRatio = 0.98f;
    float maximumAbsoluteToEngineRatio = 1.02f;
    float minimumEngineToLinearRatio = 0.98f;
    float maximumEngineToLinearRatio = 1.02f;
    float minimumEngineToLocalRatio = 0.98f;
    float maximumEngineToLocalRatio = 1.02f;
    float minimumWorldUnitsPerMeter = 0.10f;
    float maximumWorldUnitsPerMeter = 10.0f;
};

bool validWorldMetricCalibration(
    const WorldMetricCalibration& calibration
);

WorldMetricCalibration promoteMotionScaleToWorldMetric(
    const MotionScaleSnapshot& observation,
    bool absoluteSpeedMetersPerSecondSourceVerified,
    MotionMetricCalibrationTuning tuning = {}
);

std::optional<float> worldUnitsToMeters(
    float worldUnits,
    const WorldMetricCalibration& calibration
);

std::optional<float> metersToWorldUnits(
    float meters,
    const WorldMetricCalibration& calibration
);

} // namespace frr::domain
