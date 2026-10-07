#pragma once

#include <optional>

namespace frr::domain {

struct WorldMetricCalibration {
    // A numeric scale is not enough. It must be explicitly promoted only
    // after target-machine captures prove the conversion.
    bool verified = false;
    float worldUnitsPerMeter = 0.0f;
};

bool validWorldMetricCalibration(
    const WorldMetricCalibration& calibration
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
