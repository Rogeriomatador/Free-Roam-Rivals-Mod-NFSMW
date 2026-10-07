#pragma once

#include "SpawnSafety.h"
#include "StagingPlanner.h"
#include "WorldMetricCalibration.h"

#include <cstdint>

namespace frr::domain {

enum class RoadCandidateSource {
    CurrentRoad,
    FutureRoad,
    SeekAhead,
    FarFuture
};

enum class RoadCandidateBlocker {
    None,
    PositionUnavailable,
    RoadGeometryUnavailable,
    RoadGeometryAssociationUnverified,
    RoadInvalid,
    DeadEnd,
    NotAhead,
    MetricCalibrationUnverified,
    MetricConversionFailed,
    StreamingUnverified,
    GroundUnverified,
    GroundInvalid,
    VisibilityUnverified,
    VisibleToPlayer,
    OverlapUnverified,
    VehicleOverlap,
    JunctionUnverified,
    Junction,
    ObstructionUnverified,
    Obstructed,
    GradeUnverified,
    TwoCarGeometryUnverified,
    TwoCarGeometryInvalid
};

struct RoadCandidateObservation {
    RoadCandidateSource source =
        RoadCandidateSource::CurrentRoad;

    bool positionAvailable = false;
    bool roadGeometryAvailable = false;

    // True only when the sampled position is the position owned by the same
    // WRoadNav object whose segment/lane/width/curvature are attached here.
    // SeekAhead/FarFuture are intentionally false until the engine supplies
    // an exact road-nav query at those positions.
    bool exactRoadGeometry = false;

    bool roadValid = false;
    bool deadEnd = false;
    bool occludedFromBehind = false;

    std::int32_t segmentIndex = -1;
    std::int32_t laneIndex = -1;
    std::int32_t roadOcclusion = 0;
    std::int32_t avoidableOcclusion = 0;

    float distanceWorldUnits = 0.0f;
    float forwardProjectionWorldUnits = 0.0f;
    float roadWidthWorldUnits = 0.0f;
    float segmentSpanWorldUnits = 0.0f;
    float absoluteCurvature = 0.0f;
};

struct RoadCandidateEvidence {
    bool streamingVerified = false;

    bool groundVerified = false;
    bool groundValid = false;

    bool visibilityVerified = false;
    bool visibleToPlayer = true;

    bool overlapVerified = false;
    bool overlapsLiveVehicle = true;

    bool junctionVerified = false;
    bool junction = true;

    bool obstructionVerified = false;
    bool obstructed = true;

    bool gradeVerified = false;
    float absoluteGrade = 0.0f;

    bool twoCarGeometryVerified = false;
    bool supportsTwoCars = false;
};

struct RoadCandidateMetricView {
    bool available = false;
    float distanceMeters = 0.0f;
    float roadWidthMeters = 0.0f;
    float segmentSpanMeters = 0.0f;
};

struct RoadSpawnPromotion {
    bool promotable = false;
    RoadCandidateBlocker blocker =
        RoadCandidateBlocker::PositionUnavailable;

    RoadCandidateMetricView metric{};
    SpawnCandidateInput candidate{};
};

struct RoadStagingPromotion {
    bool promotable = false;
    RoadCandidateBlocker blocker =
        RoadCandidateBlocker::PositionUnavailable;

    RoadCandidateMetricView metric{};
    StagingCandidate candidate{};
};

RoadCandidateBlocker inspectRoadCandidate(
    const RoadCandidateObservation& observation
);

RoadCandidateMetricView metricViewForRoadCandidate(
    const RoadCandidateObservation& observation,
    const WorldMetricCalibration& calibration
);

RoadSpawnPromotion promoteRoadCandidateForSpawn(
    const RoadCandidateObservation& observation,
    const RoadCandidateEvidence& evidence,
    const WorldMetricCalibration& calibration,
    bool vehicleAvailable
);

RoadStagingPromotion promoteRoadCandidateForStaging(
    const RoadCandidateObservation& observation,
    const RoadCandidateEvidence& evidence,
    const WorldMetricCalibration& calibration
);

const char* roadCandidateSourceName(
    RoadCandidateSource source
);

const char* roadCandidateBlockerName(
    RoadCandidateBlocker blocker
);

} // namespace frr::domain
