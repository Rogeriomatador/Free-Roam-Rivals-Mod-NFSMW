#include "MotionScaleObserver.h"

#include <algorithm>
#include <cmath>
#include <limits>

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
        return std::numeric_limits<float>::quiet_NaN();
    }

    const float value = numerator / denominator;
    return value;
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
    lastPairAccepted_ = false;
    clearWindow();
}

void MotionScaleObserver::clearWindow() {
    window_.clear();
    ratioSum_ = 0.0;
    ratioSquareSum_ = 0.0;
    speedometerToSpeedSum_ = 0.0;
    absoluteToSpeedSum_ = 0.0;
    speedToLocalSum_ = 0.0;
    speedToLinearSum_ = 0.0;
}

bool MotionScaleObserver::acceptPair(
    const MotionScaleFrame& previous,
    const MotionScaleFrame& current,
    float deltaSeconds,
    float& ratio,
    float& speedometerToSpeed,
    float& absoluteToSpeed,
    float& speedToLocal,
    float& speedToLinear
) const {
    const auto finiteFrame = [](const MotionScaleFrame& f) {
        return std::isfinite(f.x) && std::isfinite(f.y) && std::isfinite(f.z) &&
            std::isfinite(f.engineSpeed) && std::isfinite(f.speedometer) &&
            std::isfinite(f.absoluteSpeed) && std::isfinite(f.localVelocityMagnitude) &&
            std::isfinite(f.linearVelocityMagnitude) &&
            f.localVelocityMagnitude > 0.0001f && f.linearVelocityMagnitude > 0.0001f;
    };
    if (!previous.valid ||
        !current.valid ||
        !finiteFrame(previous) || !finiteFrame(current) ||
        previous.engineSpeed <= 0.0f || current.engineSpeed <= 0.0f ||
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
        previousSpeed * 0.5f + currentSpeed * 0.5f;

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

    const float speedometerAverage =
        std::abs(previous.speedometer) * 0.5f +
        std::abs(current.speedometer) * 0.5f;

    const float absoluteAverage =
        std::abs(previous.absoluteSpeed) * 0.5f +
        std::abs(current.absoluteSpeed) * 0.5f;

    speedometerToSpeed =
        safeRatio(speedometerAverage, averageSpeed);

    absoluteToSpeed =
        safeRatio(absoluteAverage, averageSpeed);

    const float localAverage =
        previous.localVelocityMagnitude * 0.5f +
        current.localVelocityMagnitude * 0.5f;

    const float linearAverage =
        previous.linearVelocityMagnitude * 0.5f +
        current.linearVelocityMagnitude * 0.5f;

    speedToLocal =
        safeRatio(averageSpeed, localAverage);

    speedToLinear =
        safeRatio(averageSpeed, linearAverage);

    return std::isfinite(speedometerToSpeed) && std::isfinite(absoluteToSpeed) &&
        std::isfinite(speedToLocal) && std::isfinite(speedToLinear) &&
        speedToLocal > 0.0f && speedToLinear > 0.0f;
}

MotionScaleSnapshot MotionScaleObserver::push(
    const MotionScaleFrame& frame,
    float deltaSeconds
) {
    lastPairAccepted_ = false;
    if (!havePrevious_) {
        previous_ = frame;
        havePrevious_ = frame.valid;
        return snapshot();
    }

    float ratio = 0.0f;
    float speedometerToSpeed = 0.0f;
    float absoluteToSpeed = 0.0f;
    float speedToLocal = 0.0f;
    float speedToLinear = 0.0f;

    if (acceptPair(
            previous_,
            frame,
            deltaSeconds,
            ratio,
            speedometerToSpeed,
            absoluteToSpeed,
            speedToLocal,
            speedToLinear)) {
        ++acceptedSamples_;
        lastPairAccepted_ = true;
        window_.push_back({ratio, speedometerToSpeed, absoluteToSpeed, speedToLocal, speedToLinear});
        ratioSum_ += ratio;
        ratioSquareSum_ +=
            static_cast<double>(ratio) *
            static_cast<double>(ratio);
        speedometerToSpeedSum_ +=
            speedometerToSpeed;
        absoluteToSpeedSum_ +=
            absoluteToSpeed;
        speedToLocalSum_ += speedToLocal;
        speedToLinearSum_ += speedToLinear;
        const auto limit = std::clamp(tuning_.maximumWindowSamples, 1u, 4096u);
        if (window_.size() > limit) {
            const auto old = window_.front();
            window_.pop_front();
            ratioSum_ -= old[0];
            ratioSquareSum_ -= old[0] * old[0];
            speedometerToSpeedSum_ -= old[1];
            absoluteToSpeedSum_ -= old[2];
            speedToLocalSum_ -= old[3];
            speedToLinearSum_ -= old[4];
        }
    } else {
        ++rejectedSamples_;
        // Menus, jumps, gaps and invalid channels revoke old stability.
        clearWindow();
    }

    previous_ = frame;
    havePrevious_ = frame.valid;

    return snapshot();
}

MotionScaleSnapshot MotionScaleObserver::snapshot() const {
    MotionScaleSnapshot out{};
    out.acceptedSamples = acceptedSamples_;
    out.rejectedSamples = rejectedSamples_;
    out.windowSamples = static_cast<unsigned>(window_.size());
    out.lastPairAccepted = lastPairAccepted_;

    if (window_.empty()) {
        return out;
    }

    const double count =
        static_cast<double>(window_.size());

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

    out.meanSpeedometerToEngineSpeedRatio =
        static_cast<float>(
            speedometerToSpeedSum_ / count
        );

    out.meanAbsoluteToEngineSpeedRatio =
        static_cast<float>(
            absoluteToSpeedSum_ / count
        );

    out.meanSpeedToLocalVelocityRatio =
        static_cast<float>(
            speedToLocalSum_ / count
        );

    out.meanSpeedToLinearVelocityRatio =
        static_cast<float>(
            speedToLinearSum_ / count
        );

    out.stable =
        out.windowSamples >=
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
