#include "WorldMetricCalibration.h"

#include <algorithm>
#include <cmath>

namespace frr::domain {
namespace {

bool inClosedRange(
    float value,
    float minimum,
    float maximum
) {
    return
        std::isfinite(value) &&
        std::isfinite(minimum) &&
        std::isfinite(maximum) &&
        minimum <= maximum &&
        value >= minimum &&
        value <= maximum;
}

} // namespace

bool validWorldMetricCalibration(
    const WorldMetricCalibration& calibration
) {
    return
        calibration.verified &&
        std::isfinite(calibration.worldUnitsPerMeter) &&
        calibration.worldUnitsPerMeter > 0.0f;
}

WorldMetricCalibration promoteMotionScaleToWorldMetric(
    const MotionScaleSnapshot& observation,
    bool absoluteSpeedMetersPerSecondSourceVerified,
    MotionMetricCalibrationTuning tuning
) {
    WorldMetricCalibration out{};

    if (!absoluteSpeedMetersPerSecondSourceVerified ||
        !observation.stable ||
        observation.windowSamples <
            std::max(tuning.minimumStableSamples, 1u) ||
        !std::isfinite(observation.coefficientOfVariation) ||
        observation.coefficientOfVariation >
            std::max(tuning.maximumCoefficientOfVariation, 0.0f) ||
        !inClosedRange(
            observation.meanAbsoluteToEngineSpeedRatio,
            tuning.minimumAbsoluteToEngineRatio,
            tuning.maximumAbsoluteToEngineRatio) ||
        !inClosedRange(
            observation.meanSpeedToLinearVelocityRatio,
            tuning.minimumEngineToLinearRatio,
            tuning.maximumEngineToLinearRatio) ||
        !inClosedRange(
            observation.meanSpeedToLocalVelocityRatio,
            tuning.minimumEngineToLocalRatio,
            tuning.maximumEngineToLocalRatio)) {
        return out;
    }

    const float worldUnitsPerMeter =
        observation.meanWorldUnitsPerSpeedUnitSecond /
        observation.meanAbsoluteToEngineSpeedRatio;

    if (!inClosedRange(
            worldUnitsPerMeter,
            tuning.minimumWorldUnitsPerMeter,
            tuning.maximumWorldUnitsPerMeter)) {
        return out;
    }

    out.verified = true;
    out.worldUnitsPerMeter = worldUnitsPerMeter;
    return out;
}

std::optional<float> worldUnitsToMeters(
    float worldUnits,
    const WorldMetricCalibration& calibration
) {
    if (!validWorldMetricCalibration(calibration) ||
        !std::isfinite(worldUnits) ||
        worldUnits < 0.0f) {
        return std::nullopt;
    }

    const float meters =
        worldUnits / calibration.worldUnitsPerMeter;

    if (!std::isfinite(meters) || meters < 0.0f) {
        return std::nullopt;
    }

    return meters;
}

std::optional<float> metersToWorldUnits(
    float meters,
    const WorldMetricCalibration& calibration
) {
    if (!validWorldMetricCalibration(calibration) ||
        !std::isfinite(meters) ||
        meters < 0.0f) {
        return std::nullopt;
    }

    const float worldUnits =
        meters * calibration.worldUnitsPerMeter;

    if (!std::isfinite(worldUnits) || worldUnits < 0.0f) {
        return std::nullopt;
    }

    return worldUnits;
}

} // namespace frr::domain
