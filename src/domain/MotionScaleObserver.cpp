#include "MotionScaleObserver.h"

#include <algorithm>
#include <cmath>

namespace frr::domain {
namespace {

float magnitude3(
    float x,
    float y,
    float z
) {
    const float value =
        std::sqrt(x * x + y * y + z * z);

    return std::isfinite(value)
        ? value
        : 0.0f;
}

float safeRatio(
    float numerator,
    float denominator
) {
    if (!std::isfinite(numerator) ||
        !std::isfinite(denominator) ||
        denominator <= 0.0001f) {
        return 0.0f;
    }

    const float value = numerator / denominator;
    return std::isfinite(value) ? value : 0.0f;
}

} // namespace

MotionScaleObserver::MotionScaleObserver(
    MotionScaleTuning tuning
) : tuning_(tuning) {}

void MotionScaleObserver::reset() {
    havePrevious_ = false;
    previous_ = {};
    acceptedSamples_ = 0;
    rejectedSamples_ = 0;
    ratioSum_ = 0.0;
    ratioSquareSum_ = 0.0;
    speedToLocalSum_ = 0.0;
    speedToLinearSum_ = 0.0;
}

bool MotionScaleObserver::acceptPair(
    const MotionScaleFrame& previous,
    const MotionScaleFrame& current,
    float deltaSeconds,
    float& ratio,
    float& speedToLocal,
    float& speedToLinear
) const {
    if (!previous.valid ||
        !current.valid ||
        !previous.safeFreeRoam ||
        !current.safeFreeRoam ||
        !previous.grounded ||
        !current.grounded ||
        !std::isfinite(deltaSeconds) ||
        deltaSeconds <
            std::max(tuning_.minimumDeltaSeconds, 0.0f) ||
        deltaSeconds >
            std::max(
                tuning_.maximumDeltaSeconds,
                tuning_.minimumDeltaSeconds
            )) {
        return false;
    }

    const float previousSpeed =
        std::abs(previous.engineSpeed);
    const float currentSpeed =
        std::abs(current.engineSpeed);
    const float averageSpeed =
        (previousSpeed + currentSpeed) * 0.5f;

    if (!std::isfinite(averageSpeed) ||
        averageSpeed <
            std::max(tuning_.minimumEngineSpeed, 0.0f)) {
        return false;
    }

    const float speedDelta =
        std::abs(currentSpeed - previousSpeed);

    if (speedDelta / averageSpeed >
        std::max(tuning_.maximumRelativeSpeedChange, 0.0f)) {
        return false;
    }

    const float dx = current.x - previous.x;
    const float dy = current.y - previous.y;
    const float dz = current.z - previous.z;

    const float worldDistance =
        magnitude3(dx, dy, dz);

    if (worldDistance <
        std::max(tuning_.minimumWorldDistance, 0.0f)) {
        return false;
    }

    const float speedUnitDistance =
        averageSpeed * deltaSeconds;

    if (!std::isfinite(speedUnitDistance) ||
        speedUnitDistance <= 0.0001f) {
        return false;
    }

    ratio =
        worldDistance / speedUnitDistance;

    if (!std::isfinite(ratio) || ratio <= 0.0f) {
        return false;
    }

    const float localAverage =
        (previous.localVelocityMagnitude +
         current.localVelocityMagnitude) * 0.5f;

    const float linearAverage =
        (previous.linearVelocityMagnitude +
         current.linearVelocityMagnitude) * 0.5f;

    speedToLocal =
        safeRatio(averageSpeed, localAverage);

    speedToLinear =
        safeRatio(averageSpeed, linearAverage);

    return true;
}

MotionScaleSnapshot MotionScaleObserver::push(
    const MotionScaleFrame& frame,
    float deltaSeconds
) {
    if (!havePrevious_) {
        previous_ = frame;
        havePrevious_ = frame.valid;
        return snapshot();
    }

    float ratio = 0.0f;
    float speedToLocal = 0.0f;
    float speedToLinear = 0.0f;

    if (acceptPair(
            previous_,
            frame,
            deltaSeconds,
            ratio,
            speedToLocal,
            speedToLinear)) {
        ++acceptedSamples_;
        ratioSum_ += ratio;
        ratioSquareSum_ +=
            static_cast<double>(ratio) *
            static_cast<double>(ratio);
        speedToLocalSum_ += speedToLocal;
        speedToLinearSum_ += speedToLinear;
    } else {
        ++rejectedSamples_;
    }

    previous_ = frame;
    havePrevious_ = frame.valid;

    return snapshot();
}

MotionScaleSnapshot MotionScaleObserver::snapshot() const {
    MotionScaleSnapshot out{};
    out.acceptedSamples = acceptedSamples_;
    out.rejectedSamples = rejectedSamples_;

    if (acceptedSamples_ == 0) {
        return out;
    }

    const double count =
        static_cast<double>(acceptedSamples_);

    const double mean =
        ratioSum_ / count;

    const double variance = std::max(
        0.0,
        ratioSquareSum_ / count - mean * mean
    );

    const double stddev =
        std::sqrt(variance);

    out.meanWorldUnitsPerSpeedUnitSecond =
        static_cast<float>(mean);

    out.coefficientOfVariation =
        mean > 0.0
        ? static_cast<float>(stddev / mean)
        : 0.0f;

    out.meanSpeedToLocalVelocityRatio =
        static_cast<float>(
            speedToLocalSum_ / count
        );

    out.meanSpeedToLinearVelocityRatio =
        static_cast<float>(
            speedToLinearSum_ / count
        );

    out.stable =
        acceptedSamples_ >=
            std::max(
                tuning_.minimumStableSamples,
                1u
            ) &&
        out.coefficientOfVariation <=
            std::max(
                tuning_.maximumCoefficientOfVariation,
                0.0f
            );

    return out;
}

} // namespace frr::domain
