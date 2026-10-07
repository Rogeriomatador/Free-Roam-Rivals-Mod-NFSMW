#include "RoadCandidatePlanner.h"

#include <cmath>

namespace frr::domain {
namespace {

RoadCandidateBlocker metricBlocker(
    const RoadCandidateObservation& observation,
    const WorldMetricCalibration& calibration,
    RoadCandidateMetricView& metric
) {
    const auto inspected =
        inspectRoadCandidate(observation);

    if (inspected != RoadCandidateBlocker::None) {
        return inspected;
    }

    if (!validWorldMetricCalibration(calibration)) {
        return RoadCandidateBlocker::
            MetricCalibrationUnverified;
    }

    metric =
        metricViewForRoadCandidate(
            observation,
            calibration
        );

    if (!metric.available) {
        return RoadCandidateBlocker::
            MetricConversionFailed;
    }

    return RoadCandidateBlocker::None;
}

RoadCandidateBlocker commonExternalEvidenceBlocker(
    const RoadCandidateEvidence& evidence
) {
    if (!evidence.streamingVerified) {
        return RoadCandidateBlocker::StreamingUnverified;
    }

    if (!evidence.groundVerified ||
        !evidence.groundValid) {
        return RoadCandidateBlocker::GroundUnverified;
    }

    if (!evidence.visibilityVerified) {
        return RoadCandidateBlocker::VisibilityUnverified;
    }

    if (evidence.visibleToPlayer) {
        return RoadCandidateBlocker::VisibleToPlayer;
    }

    if (!evidence.overlapVerified) {
        return RoadCandidateBlocker::OverlapUnverified;
    }

    if (evidence.overlapsLiveVehicle) {
        return RoadCandidateBlocker::VehicleOverlap;
    }

    return RoadCandidateBlocker::None;
}

} // namespace

RoadCandidateBlocker inspectRoadCandidate(
    const RoadCandidateObservation& observation
) {
    if (!observation.positionAvailable ||
        !std::isfinite(observation.distanceWorldUnits) ||
        !std::isfinite(
            observation.forwardProjectionWorldUnits)) {
        return RoadCandidateBlocker::PositionUnavailable;
    }

    if (!observation.roadGeometryAvailable) {
        return RoadCandidateBlocker::
            RoadGeometryUnavailable;
    }

    if (!observation.exactRoadGeometry) {
        return RoadCandidateBlocker::
            RoadGeometryAssociationUnverified;
    }

    if (!observation.roadValid) {
        return RoadCandidateBlocker::RoadInvalid;
    }

    if (observation.deadEnd) {
        return RoadCandidateBlocker::DeadEnd;
    }

    if (observation.forwardProjectionWorldUnits <= 0.0f) {
        return RoadCandidateBlocker::NotAhead;
    }

    return RoadCandidateBlocker::None;
}

RoadCandidateMetricView metricViewForRoadCandidate(
    const RoadCandidateObservation& observation,
    const WorldMetricCalibration& calibration
) {
    RoadCandidateMetricView out{};

    const auto distance =
        worldUnitsToMeters(
            observation.distanceWorldUnits,
            calibration
        );

    const auto width =
        worldUnitsToMeters(
            observation.roadWidthWorldUnits,
            calibration
        );

    const auto span =
        worldUnitsToMeters(
            observation.segmentSpanWorldUnits,
            calibration
        );

    if (!distance || !width || !span) {
        return out;
    }

    out.available = true;
    out.distanceMeters = *distance;
    out.roadWidthMeters = *width;
    out.segmentSpanMeters = *span;
    return out;
}

RoadSpawnPromotion promoteRoadCandidateForSpawn(
    const RoadCandidateObservation& observation,
    const RoadCandidateEvidence& evidence,
    const WorldMetricCalibration& calibration,
    bool vehicleAvailable
) {
    RoadSpawnPromotion out{};

    out.blocker =
        metricBlocker(
            observation,
            calibration,
            out.metric
        );

    if (out.blocker != RoadCandidateBlocker::None) {
        return out;
    }

    out.blocker =
        commonExternalEvidenceBlocker(evidence);

    if (out.blocker != RoadCandidateBlocker::None) {
        return out;
    }

    out.candidate.available = true;
    out.candidate.vehicleAvailable =
        vehicleAvailable;
    out.candidate.roadValid = observation.roadValid;
    out.candidate.groundValid =
        evidence.groundValid;
    out.candidate.overlapsLiveVehicle =
        evidence.overlapsLiveVehicle;
    out.candidate.visibleToPlayer =
        evidence.visibleToPlayer;
    out.candidate.streamingVerified =
        evidence.streamingVerified;
    out.candidate.metricDistanceVerified = true;
    out.candidate.distanceFromPlayerMeters =
        out.metric.distanceMeters;

    out.promotable = true;
    out.blocker = RoadCandidateBlocker::None;
    return out;
}

RoadStagingPromotion promoteRoadCandidateForStaging(
    const RoadCandidateObservation& observation,
    const RoadCandidateEvidence& evidence,
    const WorldMetricCalibration& calibration
) {
    RoadStagingPromotion out{};

    out.blocker =
        metricBlocker(
            observation,
            calibration,
            out.metric
        );

    if (out.blocker != RoadCandidateBlocker::None) {
        return out;
    }

    out.blocker =
        commonExternalEvidenceBlocker(evidence);

    if (out.blocker != RoadCandidateBlocker::None) {
        return out;
    }

    if (!evidence.junctionVerified) {
        out.blocker =
            RoadCandidateBlocker::JunctionUnverified;
        return out;
    }

    if (evidence.junction) {
        out.blocker = RoadCandidateBlocker::Junction;
        return out;
    }

    if (!evidence.obstructionVerified) {
        out.blocker =
            RoadCandidateBlocker::ObstructionUnverified;
        return out;
    }

    if (evidence.obstructed) {
        out.blocker = RoadCandidateBlocker::Obstructed;
        return out;
    }

    if (!evidence.gradeVerified ||
        !std::isfinite(evidence.absoluteGrade) ||
        evidence.absoluteGrade < 0.0f) {
        out.blocker =
            RoadCandidateBlocker::GradeUnverified;
        return out;
    }

    if (!evidence.twoCarGeometryVerified) {
        out.blocker =
            RoadCandidateBlocker::
                TwoCarGeometryUnverified;
        return out;
    }

    if (!evidence.supportsTwoCars) {
        out.blocker =
            RoadCandidateBlocker::
                TwoCarGeometryInvalid;
        return out;
    }

    out.candidate.metricGeometryVerified = true;
    out.candidate.distanceAheadMeters =
        out.metric.distanceMeters;
    out.candidate.roadWidthMeters =
        out.metric.roadWidthMeters;
    out.candidate.absoluteCurvature =
        std::abs(observation.absoluteCurvature);
    out.candidate.absoluteGrade =
        evidence.absoluteGrade;
    out.candidate.roadValid =
        observation.roadValid;
    out.candidate.streamed =
        evidence.streamingVerified;
    out.candidate.junction =
        evidence.junction;
    out.candidate.obstructed =
        evidence.obstructed;
    out.candidate.groundValid =
        evidence.groundValid;
    out.candidate.supportsTwoCars =
        evidence.supportsTwoCars;

    out.promotable = true;
    out.blocker = RoadCandidateBlocker::None;
    return out;
}

const char* roadCandidateSourceName(
    RoadCandidateSource source
) {
    switch (source) {
        case RoadCandidateSource::CurrentRoad:
            return "CurrentRoad";
        case RoadCandidateSource::FutureRoad:
            return "FutureRoad";
        case RoadCandidateSource::SeekAhead:
            return "SeekAhead";
        case RoadCandidateSource::FarFuture:
            return "FarFuture";
    }

    return "Unknown";
}

const char* roadCandidateBlockerName(
    RoadCandidateBlocker blocker
) {
    switch (blocker) {
        case RoadCandidateBlocker::None:
            return "None";
        case RoadCandidateBlocker::PositionUnavailable:
            return "PositionUnavailable";
        case RoadCandidateBlocker::RoadGeometryUnavailable:
            return "RoadGeometryUnavailable";
        case RoadCandidateBlocker::RoadGeometryAssociationUnverified:
            return "RoadGeometryAssociationUnverified";
        case RoadCandidateBlocker::RoadInvalid:
            return "RoadInvalid";
        case RoadCandidateBlocker::DeadEnd:
            return "DeadEnd";
        case RoadCandidateBlocker::NotAhead:
            return "NotAhead";
        case RoadCandidateBlocker::MetricCalibrationUnverified:
            return "MetricCalibrationUnverified";
        case RoadCandidateBlocker::MetricConversionFailed:
            return "MetricConversionFailed";
        case RoadCandidateBlocker::StreamingUnverified:
            return "StreamingUnverified";
        case RoadCandidateBlocker::GroundUnverified:
            return "GroundUnverified";
        case RoadCandidateBlocker::VisibilityUnverified:
            return "VisibilityUnverified";
        case RoadCandidateBlocker::VisibleToPlayer:
            return "VisibleToPlayer";
        case RoadCandidateBlocker::OverlapUnverified:
            return "OverlapUnverified";
        case RoadCandidateBlocker::VehicleOverlap:
            return "VehicleOverlap";
        case RoadCandidateBlocker::JunctionUnverified:
            return "JunctionUnverified";
        case RoadCandidateBlocker::Junction:
            return "Junction";
        case RoadCandidateBlocker::ObstructionUnverified:
            return "ObstructionUnverified";
        case RoadCandidateBlocker::Obstructed:
            return "Obstructed";
        case RoadCandidateBlocker::GradeUnverified:
            return "GradeUnverified";
        case RoadCandidateBlocker::TwoCarGeometryUnverified:
            return "TwoCarGeometryUnverified";
        case RoadCandidateBlocker::TwoCarGeometryInvalid:
            return "TwoCarGeometryInvalid";
    }

    return "Unknown";
}

} // namespace frr::domain
