#include "RoadCandidateProbe.h"

#include <cmath>

namespace frr::game {
namespace {

frr::domain::RoadCandidateVector3 copyCandidateVector(
    const RoadVectorProbe& value
) {
    frr::domain::RoadCandidateVector3 out{};
    out.x = value.x;
    out.y = value.y;
    out.z = value.z;
    out.finite = value.finite;
    return out;
}

float distance(
    const RoadVectorProbe& a,
    const RoadVectorProbe& b
) {
    if (!a.finite || !b.finite) {
        return 0.0f;
    }

    const float dx = a.x - b.x;
    const float dy = a.y - b.y;
    const float dz = a.z - b.z;

    const float value = std::sqrt(
        dx * dx + dy * dy + dz * dz
    );

    return std::isfinite(value) ? value : 0.0f;
}

float projection(
    const RoadVectorProbe& origin,
    const RoadVectorProbe& target,
    const RoadVectorProbe& forward
) {
    if (!origin.finite ||
        !target.finite ||
        !forward.finite) {
        return 0.0f;
    }

    const float length = std::sqrt(
        forward.x * forward.x +
        forward.y * forward.y +
        forward.z * forward.z
    );

    if (!std::isfinite(length) ||
        length <= 0.0001f) {
        return 0.0f;
    }

    const float dx = target.x - origin.x;
    const float dy = target.y - origin.y;
    const float dz = target.z - origin.z;

    const float value =
        dx * (forward.x / length) +
        dy * (forward.y / length) +
        dz * (forward.z / length);

    return std::isfinite(value) ? value : 0.0f;
}

frr::domain::RoadCandidateObservation fromRoad(
    frr::domain::RoadCandidateSource source,
    const PlayerRoadNavigationProbe& navigation,
    const RoadNavPointProbe& road
) {
    frr::domain::RoadCandidateObservation out{};
    out.source = source;
    out.positionAvailable =
        road.position.finite;
    out.roadGeometryAvailable =
        road.available;
    out.exactRoadGeometry = true;
    out.roadValid = road.valid;
    out.deadEnd = road.deadEnd;
    out.occludedFromBehind =
        road.occludedFromBehind;
    out.segmentIndex = road.segmentIndex;
    out.laneIndex = road.laneIndex;
    out.roadOcclusion = road.roadOcclusion;
    out.avoidableOcclusion =
        road.avoidableOcclusion;
    out.distanceWorldUnits = distance(
        navigation.playerPosition,
        road.position
    );

    const RoadVectorProbe& forward =
        navigation.current.forward.finite
        ? navigation.current.forward
        : road.forward;

    out.forwardProjectionWorldUnits =
        projection(
            navigation.playerPosition,
            road.position,
            forward
        );

    out.roadWidthWorldUnits =
        road.roadWidthWorldUnits;
    out.segmentSpanWorldUnits =
        road.segmentSpanWorldUnits;
    out.absoluteCurvature =
        std::abs(road.curvature);
    out.position =
        copyCandidateVector(road.position);
    out.forward =
        copyCandidateVector(road.forward);
    return out;
}

frr::domain::RoadCandidateObservation fromLookahead(
    frr::domain::RoadCandidateSource source,
    const PlayerRoadNavigationProbe& navigation,
    const RoadVectorProbe& position,
    const RoadVectorProbe* explicitForward,
    float distanceWorldUnits,
    float projectionWorldUnits
) {
    const RoadNavPointProbe* geometry = nullptr;

    if (navigation.future.available) {
        geometry = &navigation.future;
    } else if (navigation.current.available) {
        geometry = &navigation.current;
    }

    frr::domain::RoadCandidateObservation out{};
    out.source = source;
    out.positionAvailable = position.finite;

    if (geometry) {
        out.roadGeometryAvailable = true;
        out.roadValid = geometry->valid;
        out.deadEnd = geometry->deadEnd;
        out.occludedFromBehind =
            geometry->occludedFromBehind;
        out.segmentIndex = geometry->segmentIndex;
        out.laneIndex = geometry->laneIndex;
        out.roadOcclusion = geometry->roadOcclusion;
        out.avoidableOcclusion =
            geometry->avoidableOcclusion;
        out.roadWidthWorldUnits =
            geometry->roadWidthWorldUnits;
        out.segmentSpanWorldUnits =
            geometry->segmentSpanWorldUnits;
        out.absoluteCurvature =
            std::abs(geometry->curvature);
    }

    out.position =
        copyCandidateVector(position);

    if (explicitForward && explicitForward->finite) {
        out.forward =
            copyCandidateVector(*explicitForward);
    } else if (geometry) {
        out.forward =
            copyCandidateVector(geometry->forward);
    }

    // The AI exposes these positions, but no verified API currently tells us
    // that they belong to exactly the same WRoadNav object copied above.
    out.exactRoadGeometry = false;
    out.distanceWorldUnits =
        distanceWorldUnits;
    out.forwardProjectionWorldUnits =
        projectionWorldUnits;
    return out;
}

} // namespace

std::vector<frr::domain::RoadCandidateObservation>
RoadCandidateProbe::build(
    const PlayerRoadNavigationProbe& roadNavigation
) {
    using frr::domain::RoadCandidateSource;

    std::vector<frr::domain::RoadCandidateObservation> out;

    if (!roadNavigation.available) {
        return out;
    }

    out.reserve(4);

    if (roadNavigation.current.available) {
        out.push_back(
            fromRoad(
                RoadCandidateSource::CurrentRoad,
                roadNavigation,
                roadNavigation.current
            )
        );
    }

    if (roadNavigation.future.available) {
        out.push_back(
            fromRoad(
                RoadCandidateSource::FutureRoad,
                roadNavigation,
                roadNavigation.future
            )
        );
    }

    if (roadNavigation.seekAheadPosition.finite) {
        out.push_back(
            fromLookahead(
                RoadCandidateSource::SeekAhead,
                roadNavigation,
                roadNavigation.seekAheadPosition,
                nullptr,
                roadNavigation.seekAheadDistanceWorldUnits,
                roadNavigation.seekAheadProjectionWorldUnits
            )
        );
    }

    if (roadNavigation.farFuturePosition.finite) {
        out.push_back(
            fromLookahead(
                RoadCandidateSource::FarFuture,
                roadNavigation,
                roadNavigation.farFuturePosition,
                &roadNavigation.farFutureDirection,
                roadNavigation.farFutureDistanceWorldUnits,
                roadNavigation.farFutureProjectionWorldUnits
            )
        );
    }

    return out;
}

} // namespace frr::game
