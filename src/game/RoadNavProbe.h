#pragma once

#include <cstdint>

namespace frr::game {

struct RoadVectorProbe {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    bool finite = false;
};

struct RoadNavPointProbe {
    bool available = false;
    bool valid = false;
    bool deadEnd = false;
    bool occludedFromBehind = false;

    std::uintptr_t address = 0;
    std::int32_t segmentIndex = -1;
    std::int32_t laneIndex = -1;

    std::int32_t roadOcclusion = 0;
    std::int32_t avoidableOcclusion = 0;

    float segmentTime = 0.0f;
    float curvature = 0.0f;

    // These are engine/world coordinate units. Do not treat them as metres
    // until an explicit target-machine calibration proves the scale.
    float roadWidthWorldUnits = 0.0f;
    float segmentSpanWorldUnits = 0.0f;

    RoadVectorProbe position{};
    RoadVectorProbe forward{};
    RoadVectorProbe leftPosition{};
    RoadVectorProbe rightPosition{};
    RoadVectorProbe startPosition{};
    RoadVectorProbe endPosition{};
};

struct PlayerRoadNavigationProbe {
    bool available = false;
    bool playerAiAvailable = false;

    std::uintptr_t playerAi = 0;

    RoadVectorProbe playerPosition{};

    RoadNavPointProbe current{};
    RoadNavPointProbe future{};

    RoadVectorProbe seekAheadPosition{};
    RoadVectorProbe farFuturePosition{};
    RoadVectorProbe farFutureDirection{};

    float currentToFutureWorldUnits = 0.0f;
    float seekAheadDistanceWorldUnits = 0.0f;
    float seekAheadProjectionWorldUnits = 0.0f;
    float farFutureDistanceWorldUnits = 0.0f;
    float farFutureProjectionWorldUnits = 0.0f;
};

class RoadNavProbe {
public:
    static PlayerRoadNavigationProbe sample(
        std::uintptr_t playerPVehicle
    );
};

} // namespace frr::game
