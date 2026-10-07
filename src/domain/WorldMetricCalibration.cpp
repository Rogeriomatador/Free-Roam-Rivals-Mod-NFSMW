#include "WorldMetricCalibration.h"

#include <cmath>

namespace frr::domain {

bool validWorldMetricCalibration(
    const WorldMetricCalibration& calibration
) {
    return
        calibration.verified &&
        std::isfinite(calibration.worldUnitsPerMeter) &&
        calibration.worldUnitsPerMeter > 0.0f;
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
