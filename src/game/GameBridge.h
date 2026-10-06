#pragma once

#include <cstdint>

namespace frr::game {

enum class RacePlayMode {
    Unknown,
    Roaming,
    Racing
};

enum class WorldProbeMode {
    NoWorld,
    NoPlayerVehicle,
    Transition,
    NIS,
    StockRace,
    FreeRoamCandidate
};

struct VehicleProbe {
    bool registryReadable = false;

    std::uintptr_t playerIVehicle = 0;
    std::uintptr_t playerPVehicleCandidate = 0;
    bool playerPVehicleVtableVerified = false;

    std::uint32_t totalVehicles = 0;
    std::uint32_t humanVehicles = 0;
    std::uint32_t trafficVehicles = 0;
    std::uint32_t copVehicles = 0;
    std::uint32_t racerVehicles = 0;
    std::uint32_t noneVehicles = 0;
    std::uint32_t nisVehicles = 0;
    std::uint32_t remoteVehicles = 0;
    std::uint32_t unknownVehicles = 0;
};

struct CareerProbe {
    bool available = false;
    std::int32_t cash = 0;
    std::uint32_t careerCars = 0;
    std::uint32_t currentCarHandle = 0;
    bool careerCompletedAtLeastOnce = false;
};

struct RuntimeSnapshot {
    bool inWorld = false;
    bool inNIS = false;
    bool fadeScreen = false;

    std::uint32_t gameFlowState = 0;

    std::uintptr_t raceStatus = 0;
    bool raceStatusLoading = false;
    RacePlayMode racePlayMode = RacePlayMode::Unknown;

    std::uintptr_t roadNetwork = 0;

    VehicleProbe vehicles{};
    CareerProbe career{};

    WorldProbeMode mode = WorldProbeMode::NoWorld;
};

class GameBridge {
public:
    static RuntimeSnapshot sample();
};

const char* worldProbeModeName(WorldProbeMode mode);
const char* racePlayModeName(RacePlayMode mode);

} // namespace frr::game
