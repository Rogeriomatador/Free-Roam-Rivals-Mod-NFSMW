#pragma once

#include <cstdint>

namespace frr::game {

enum class WorldProbeMode {
    NoPlayerVehicle,
    Transition,
    NIS,
    DrivingCandidate
};

struct VehicleProbe {
    std::uintptr_t playerVehicle = 0;
    std::uint32_t totalVehicles = 0;
    std::uint32_t playerVehicles = 0;
    std::uint32_t aiVehicles = 0;
    std::uint32_t unknownVehicles = 0;
};

struct RuntimeSnapshot {
    bool inNIS = false;
    bool fadeScreen = false;

    std::uintptr_t raceStatus = 0;
    std::uintptr_t gameFlowRaw = 0;

    VehicleProbe vehicles{};
    WorldProbeMode mode = WorldProbeMode::NoPlayerVehicle;
};

class GameBridge {
public:
    static RuntimeSnapshot sample();
};

const char* worldProbeModeName(WorldProbeMode mode);

} // namespace frr::game
