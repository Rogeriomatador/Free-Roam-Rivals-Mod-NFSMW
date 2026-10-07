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

    std::uintptr_t address = 0;
    std::int32_t segmentIndex = -1;
    std::int32_t laneIndex = -1;

    float curvature = 0.0f;
    float roadWidthMeters = 0.0f;
    float segmentSpanMeters = 0.0f;

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

    RoadNavPointProbe current{};
    RoadNavPointProbe future{};

    float currentToFutureMeters = 0.0f;
};

class RoadNavProbe {
public:
    static PlayerRoadNavigationProbe sample(
        std::uintptr_t playerPVehicle
    );
};

} // namespace frr::game
