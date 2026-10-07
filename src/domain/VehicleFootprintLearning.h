#pragma once

#include "VehicleSpatialEvidence.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace frr::domain {

struct VehicleFootprintTuning {
    std::uint32_t minimumSamples = 4;
    float maximumRelativeSpread = 0.03f;
};

struct VehicleFootprintEstimate {
    bool found = false;
    bool verified = false;

    std::uint32_t vehicleKey = 0;
    std::uint32_t sampleCount = 0;

    SpatialVector3 meanHalfExtents{};
    SpatialVector3 minimumHalfExtents{};
    SpatialVector3 maximumHalfExtents{};

    float maximumObservedRelativeSpread = 0.0f;
};

class VehicleFootprintLearner {
public:
    explicit VehicleFootprintLearner(
        VehicleFootprintTuning tuning = {}
    );

    void reset();

    void observe(
        const VehicleOrientedBox& box
    );

    VehicleFootprintEstimate estimate(
        std::uint32_t vehicleKey
    ) const;

    std::size_t modelCount() const;
    std::size_t verifiedModelCount() const;

    // Public only so the implementation can build an estimate without
    // duplicating the accumulator layout. Callers should treat this as
    // implementation state.
    struct Accumulator {
        std::uint32_t vehicleKey = 0;
        std::uint32_t sampleCount = 0;

        SpatialVector3 mean{};
        SpatialVector3 minimum{};
        SpatialVector3 maximum{};
    };

private:
    VehicleFootprintTuning tuning_{};
    std::vector<Accumulator> accumulators_{};
};

VehicleOrientedBox makeRoadAlignedVehicleFootprint(
    std::uint32_t vehicleKey,
    const SpatialVector3& center,
    const SpatialVector3& forward,
    const SpatialVector3& halfExtents
);

} // namespace frr::domain
