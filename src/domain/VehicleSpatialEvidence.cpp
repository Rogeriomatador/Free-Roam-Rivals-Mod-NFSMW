#include "VehicleSpatialEvidence.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <limits>

namespace frr::domain {
namespace {

constexpr float kAxisEpsilon = 1.0e-4f;
constexpr float kSatEpsilon = 1.0e-5f;

bool finite(float v) {
    return std::isfinite(v);
}

bool finiteVector(const SpatialVector3& v) {
    return finite(v.x) && finite(v.y) && finite(v.z);
}

float dot(
    const SpatialVector3& a,
    const SpatialVector3& b
) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

SpatialVector3 subtract(
    const SpatialVector3& a,
    const SpatialVector3& b
) {
    return {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z
    };
}

float length(const SpatialVector3& v) {
    const float result = std::sqrt(dot(v, v));
    return finite(result) ? result : 0.0f;
}

bool normalize(
    const SpatialVector3& in,
    SpatialVector3& out
) {
    const float magnitude = length(in);
    if (!finite(magnitude) ||
        magnitude <= kAxisEpsilon) {
        return false;
    }

    out = {
        in.x / magnitude,
        in.y / magnitude,
        in.z / magnitude
    };
    return finiteVector(out);
}

struct NormalizedBox {
    SpatialVector3 center{};
    std::array<SpatialVector3, 3> axis{};
    std::array<float, 3> extent{};
};

bool normalizeBox(
    const VehicleOrientedBox& source,
    NormalizedBox& out
) {
    if (!source.valid ||
        !finiteVector(source.center) ||
        !finiteVector(source.right) ||
        !finiteVector(source.up) ||
        !finiteVector(source.forward) ||
        !finiteVector(source.halfExtents) ||
        source.halfExtents.x <= 0.0f ||
        source.halfExtents.y <= 0.0f ||
        source.halfExtents.z <= 0.0f) {
        return false;
    }

    out.center = source.center;
    out.extent = {
        source.halfExtents.x,
        source.halfExtents.y,
        source.halfExtents.z
    };

    if (!normalize(source.right, out.axis[0]) ||
        !normalize(source.up, out.axis[1]) ||
        !normalize(source.forward, out.axis[2])) {
        return false;
    }

    // A rigid-body basis should be close to orthogonal. Reject corrupt
    // or mismatched vtable reads instead of silently normalizing nonsense.
    if (std::abs(dot(out.axis[0], out.axis[1])) > 0.05f ||
        std::abs(dot(out.axis[0], out.axis[2])) > 0.05f ||
        std::abs(dot(out.axis[1], out.axis[2])) > 0.05f) {
        return false;
    }

    return true;
}

bool finitePoint(const SpatialVector3& p) {
    return finiteVector(p);
}

} // namespace

bool validVehicleOrientedBox(
    const VehicleOrientedBox& box
) {
    NormalizedBox normalized{};
    return normalizeBox(box, normalized);
}

VehicleOrientedBox makeVehicleOrientedBox(std::uintptr_t identity, std::uint32_t vehicleKey,
    SpatialVector3 center, SpatialVector3 right, SpatialVector3 up,
    SpatialVector3 forward, SpatialVector3 halfExtents) {
    VehicleOrientedBox box{true, identity, vehicleKey, center, right, up, forward, halfExtents};
    box.valid = validVehicleOrientedBox(box);
    return box;
}

bool pointInsideVehicleOrientedBox(
    const SpatialVector3& point,
    const VehicleOrientedBox& box
) {
    if (!finitePoint(point)) {
        return false;
    }

    NormalizedBox normalized{};
    if (!normalizeBox(box, normalized)) {
        return false;
    }

    const SpatialVector3 delta =
        subtract(point, normalized.center);

    for (int i = 0; i < 3; ++i) {
        if (std::abs(dot(delta, normalized.axis[i])) >
            normalized.extent[i]) {
            return false;
        }
    }

    return true;
}

float pointSeparationFromVehicleOrientedBox(
    const SpatialVector3& point,
    const VehicleOrientedBox& box
) {
    if (!finitePoint(point)) {
        return std::numeric_limits<float>::infinity();
    }

    NormalizedBox normalized{};
    if (!normalizeBox(box, normalized)) {
        return std::numeric_limits<float>::infinity();
    }

    const SpatialVector3 delta =
        subtract(point, normalized.center);

    float squareDistance = 0.0f;

    for (int i = 0; i < 3; ++i) {
        const float local =
            dot(delta, normalized.axis[i]);
        const float outside =
            std::max(
                std::abs(local) - normalized.extent[i],
                0.0f
            );
        squareDistance += outside * outside;
    }

    const float result = std::sqrt(squareDistance);
    return finite(result)
        ? result
        : std::numeric_limits<float>::infinity();
}

bool vehicleOrientedBoxesOverlap(
    const VehicleOrientedBox& a,
    const VehicleOrientedBox& b,
    float paddingWorldUnits
) {
    NormalizedBox A{};
    NormalizedBox B{};

    if (!normalizeBox(a, A) ||
        !normalizeBox(b, B) ||
        !finite(paddingWorldUnits) ||
        paddingWorldUnits < 0.0f) {
        return false;
    }

    std::array<float, 3> aExtent = A.extent;
    std::array<float, 3> bExtent = B.extent;

    for (int i = 0; i < 3; ++i) {
        aExtent[i] += paddingWorldUnits;
        bExtent[i] += paddingWorldUnits;
    }

    float R[3][3]{};
    float AbsR[3][3]{};

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            R[i][j] = dot(A.axis[i], B.axis[j]);
            AbsR[i][j] =
                std::abs(R[i][j]) + kSatEpsilon;
        }
    }

    const SpatialVector3 worldT =
        subtract(B.center, A.center);

    const float t[3] = {
        dot(worldT, A.axis[0]),
        dot(worldT, A.axis[1]),
        dot(worldT, A.axis[2])
    };

    float ra = 0.0f;
    float rb = 0.0f;

    // A's three local axes.
    for (int i = 0; i < 3; ++i) {
        ra = aExtent[i];
        rb =
            bExtent[0] * AbsR[i][0] +
            bExtent[1] * AbsR[i][1] +
            bExtent[2] * AbsR[i][2];

        if (std::abs(t[i]) > ra + rb) {
            return false;
        }
    }

    // B's three local axes.
    for (int j = 0; j < 3; ++j) {
        ra =
            aExtent[0] * AbsR[0][j] +
            aExtent[1] * AbsR[1][j] +
            aExtent[2] * AbsR[2][j];
        rb = bExtent[j];

        const float projectedT = std::abs(
            t[0] * R[0][j] +
            t[1] * R[1][j] +
            t[2] * R[2][j]
        );

        if (projectedT > ra + rb) {
            return false;
        }
    }

    // Nine cross-product axes Ai x Bj.
    for (int i = 0; i < 3; ++i) {
        const int i1 = (i + 1) % 3;
        const int i2 = (i + 2) % 3;

        for (int j = 0; j < 3; ++j) {
            const int j1 = (j + 1) % 3;
            const int j2 = (j + 2) % 3;

            ra =
                aExtent[i1] * AbsR[i2][j] +
                aExtent[i2] * AbsR[i1][j];

            rb =
                bExtent[j1] * AbsR[i][j2] +
                bExtent[j2] * AbsR[i][j1];

            const float projectedT = std::abs(
                t[i2] * R[i1][j] -
                t[i1] * R[i2][j]
            );

            if (projectedT > ra + rb) {
                return false;
            }
        }
    }

    return true;
}

PointOccupancyReport evaluatePointAgainstFleet(
    const SpatialVector3& point,
    const std::vector<VehicleOrientedBox>& fleet,
    bool registryComplete
) {
    PointOccupancyReport out{};
    out.registryComplete = registryComplete;

    if (!finitePoint(point)) {
        return out;
    }

    out.queryValid = true;

    float nearest =
        std::numeric_limits<float>::infinity();

    for (const auto& vehicle : fleet) {
        if (vehicle.identity == 0 ||
            !validVehicleOrientedBox(vehicle)) {
            ++out.invalidVehicles;
            continue;
        }

        ++out.checkedVehicles;

        const float separation =
            pointSeparationFromVehicleOrientedBox(
                point,
                vehicle
            );

        if (separation < nearest) {
            nearest = separation;
            out.nearestVehicle = vehicle.identity;
        }

        if (!out.insideAnyVehicle &&
            pointInsideVehicleOrientedBox(
                point,
                vehicle
            )) {
            out.insideAnyVehicle = true;
            out.containingVehicle = vehicle.identity;
        }
    }

    if (finite(nearest)) {
        out.nearestSeparationWorldUnits = nearest;
    }

    out.verified =
        out.queryValid &&
        out.registryComplete &&
        out.invalidVehicles == 0;

    return out;
}

FleetOverlapReport evaluateFootprintAgainstFleet(
    const VehicleOrientedBox& candidate,
    const std::vector<VehicleOrientedBox>& fleet,
    bool registryComplete,
    float paddingWorldUnits
) {
    FleetOverlapReport out{};
    out.registryComplete = registryComplete;
    out.candidateValid =
        validVehicleOrientedBox(candidate);

    if (!out.candidateValid ||
        !finite(paddingWorldUnits) ||
        paddingWorldUnits < 0.0f) {
        return out;
    }

    for (const auto& vehicle : fleet) {
        if (vehicle.identity == 0 ||
            !validVehicleOrientedBox(vehicle)) {
            ++out.invalidVehicles;
            continue;
        }

        ++out.checkedVehicles;

        if (!out.overlaps &&
            vehicleOrientedBoxesOverlap(
                candidate,
                vehicle,
                paddingWorldUnits
            )) {
            out.overlaps = true;
            out.overlappingVehicle =
                vehicle.identity;
        }
    }

    out.verified =
        out.candidateValid &&
        out.registryComplete &&
        out.invalidVehicles == 0;

    return out;
}

} // namespace frr::domain

