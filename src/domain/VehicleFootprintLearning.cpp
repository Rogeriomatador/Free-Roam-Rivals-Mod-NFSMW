#include "VehicleFootprintLearning.h"

#include <algorithm>
#include <cmath>
#include <limits>

namespace frr::domain {
namespace {

constexpr float kVectorEpsilon = 1.0e-4f;

bool finite(float value) {
    return std::isfinite(value);
}

bool finiteVector(
    const SpatialVector3& value
) {
    return
        finite(value.x) &&
        finite(value.y) &&
        finite(value.z);
}

float dot(
    const SpatialVector3& a,
    const SpatialVector3& b
) {
    return a.x * b.x +
           a.y * b.y +
           a.z * b.z;
}

SpatialVector3 cross(
    const SpatialVector3& a,
    const SpatialVector3& b
) {
    return {
        a.y * b.z - a.z * b.y,
        a.z * b.x - a.x * b.z,
        a.x * b.y - a.y * b.x
    };
}

bool normalize(
    const SpatialVector3& input,
    SpatialVector3& output
) {
    const float lengthSquared =
        dot(input, input);

    if (!finite(lengthSquared) ||
        lengthSquared <=
            kVectorEpsilon * kVectorEpsilon) {
        return false;
    }

    const float length =
        std::sqrt(lengthSquared);

    if (!finite(length) ||
        length <= kVectorEpsilon) {
        return false;
    }

    output = {
        input.x / length,
        input.y / length,
        input.z / length
    };

    return finiteVector(output);
}

bool validHalfExtents(
    const SpatialVector3& value
) {
    return
        finiteVector(value) &&
        value.x > 0.0f &&
        value.y > 0.0f &&
        value.z > 0.0f;
}

float relativeSpread(
    float minimum,
    float maximum,
    float mean
) {
    if (!finite(minimum) ||
        !finite(maximum) ||
        !finite(mean) ||
        mean <= kVectorEpsilon ||
        maximum < minimum) {
        return std::numeric_limits<float>::infinity();
    }

    return (maximum - minimum) / mean;
}

VehicleFootprintEstimate makeEstimate(
    const VehicleFootprintLearner::Accumulator& value,
    const VehicleFootprintTuning& tuning
) {
    VehicleFootprintEstimate out{};
    out.found = true;
    out.vehicleKey = value.vehicleKey;
    out.sampleCount = value.sampleCount;
    out.meanHalfExtents = value.mean;
    out.minimumHalfExtents = value.minimum;
    out.maximumHalfExtents = value.maximum;

    const float spreadX =
        relativeSpread(
            value.minimum.x,
            value.maximum.x,
            value.mean.x
        );
    const float spreadY =
        relativeSpread(
            value.minimum.y,
            value.maximum.y,
            value.mean.y
        );
    const float spreadZ =
        relativeSpread(
            value.minimum.z,
            value.maximum.z,
            value.mean.z
        );

    out.maximumObservedRelativeSpread =
        std::max(
            spreadX,
            std::max(spreadY, spreadZ)
        );

    out.verified =
        value.sampleCount >= tuning.minimumSamples &&
        finite(out.maximumObservedRelativeSpread) &&
        out.maximumObservedRelativeSpread <=
            tuning.maximumRelativeSpread;

    return out;
}

} // namespace

VehicleFootprintLearner::VehicleFootprintLearner(
    VehicleFootprintTuning tuning
)
    : tuning_(tuning) {
    if (tuning_.minimumSamples == 0) {
        tuning_.minimumSamples = 1;
    }

    if (!finite(tuning_.maximumRelativeSpread) ||
        tuning_.maximumRelativeSpread < 0.0f) {
        tuning_.maximumRelativeSpread = 0.0f;
    }
}

void VehicleFootprintLearner::reset() {
    accumulators_.clear();
}

void VehicleFootprintLearner::observe(
    const VehicleOrientedBox& box
) {
    if (!box.valid ||
        box.vehicleKey == 0 ||
        !validVehicleOrientedBox(box) ||
        !validHalfExtents(box.halfExtents)) {
        return;
    }

    auto found =
        std::find_if(
            accumulators_.begin(),
            accumulators_.end(),
            [&box](const Accumulator& value) {
                return value.vehicleKey ==
                    box.vehicleKey;
            }
        );

    if (found == accumulators_.end()) {
        Accumulator value{};
        value.vehicleKey = box.vehicleKey;
        value.sampleCount = 1;
        value.mean = box.halfExtents;
        value.minimum = box.halfExtents;
        value.maximum = box.halfExtents;

        accumulators_.push_back(value);
        return;
    }

    ++found->sampleCount;

    const float count =
        static_cast<float>(found->sampleCount);

    found->mean.x +=
        (box.halfExtents.x - found->mean.x) /
        count;
    found->mean.y +=
        (box.halfExtents.y - found->mean.y) /
        count;
    found->mean.z +=
        (box.halfExtents.z - found->mean.z) /
        count;

    found->minimum.x =
        std::min(
            found->minimum.x,
            box.halfExtents.x
        );
    found->minimum.y =
        std::min(
            found->minimum.y,
            box.halfExtents.y
        );
    found->minimum.z =
        std::min(
            found->minimum.z,
            box.halfExtents.z
        );

    found->maximum.x =
        std::max(
            found->maximum.x,
            box.halfExtents.x
        );
    found->maximum.y =
        std::max(
            found->maximum.y,
            box.halfExtents.y
        );
    found->maximum.z =
        std::max(
            found->maximum.z,
            box.halfExtents.z
        );
}

VehicleFootprintEstimate
VehicleFootprintLearner::estimate(
    std::uint32_t vehicleKey
) const {
    for (const auto& value : accumulators_) {
        if (value.vehicleKey == vehicleKey) {
            return makeEstimate(value, tuning_);
        }
    }

    return {};
}

std::size_t
VehicleFootprintLearner::modelCount() const {
    return accumulators_.size();
}

std::size_t
VehicleFootprintLearner::verifiedModelCount() const {
    std::size_t count = 0;

    for (const auto& value : accumulators_) {
        if (makeEstimate(value, tuning_).verified) {
            ++count;
        }
    }

    return count;
}

VehicleOrientedBox makeRoadAlignedVehicleFootprint(
    std::uint32_t vehicleKey,
    const SpatialVector3& center,
    const SpatialVector3& forward,
    const SpatialVector3& halfExtents
) {
    VehicleOrientedBox out{};

    if (vehicleKey == 0 ||
        !finiteVector(center) ||
        !finiteVector(forward) ||
        !validHalfExtents(halfExtents)) {
        return out;
    }

    SpatialVector3 normalizedForward{};
    if (!normalize(forward, normalizedForward)) {
        return out;
    }

    // NFSMW rigid-body dimensions use Y for the local vertical extent.
    // Build a road-aligned basis around world +Y. Near-vertical road
    // directions are rejected rather than inventing an orientation.
    const SpatialVector3 worldUp{
        0.0f,
        1.0f,
        0.0f
    };

    SpatialVector3 right{};
    if (!normalize(
            cross(worldUp, normalizedForward),
            right)) {
        return out;
    }

    SpatialVector3 up{};
    if (!normalize(
            cross(normalizedForward, right),
            up)) {
        return out;
    }

    out.valid = true;
    out.vehicleKey = vehicleKey;
    out.center = center;
    out.right = right;
    out.up = up;
    out.forward = normalizedForward;
    out.halfExtents = halfExtents;

    if (!validVehicleOrientedBox(out)) {
        out.valid = false;
    }

    return out;
}

} // namespace frr::domain
