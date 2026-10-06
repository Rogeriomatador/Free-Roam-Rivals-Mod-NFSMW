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

    // MWSDK's verified live vehicle registry exposes IVehicle* entries.
    // Never reinterpret these as PVehicle* or subtract a guessed subobject
    // offset: MWSDK explicitly documents that as unsafe.
    std::uintptr_t playerIVehicle = 0;

    // Independent NFSPluginSDK cross-check. This is resolved through its
    // validated PVehicle registry helper, not derived from playerIVehicle.
    std::uintptr_t playerPVehicle = 0;
    std::uint32_t pvehicleRegistryCount = 0;
    bool independentPlayerCrossCheck = false;

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

struct RuntimeCapabilities {
    bool canObserveWorld = false;
    bool canIdentifyPlayer = false;
    bool canClassifyFreeRoam = false;
    bool roadNetworkAvailable = false;
    bool careerReadAvailable = false;

    // Remains false until a dedicated experimental-spawn build proves the
    // complete create -> attach AI -> goal -> cleanup lifecycle safely.
    bool rivalSpawnExperimentVerified = false;

    // Remains false until exact engine-backed ownership transfer + rollback
    // are proven. Read access to cash/garage does NOT imply write safety.
    bool economyWriteVerified = false;
    bool garageWriteVerified = false;
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
    RuntimeCapabilities capabilities{};

    WorldProbeMode mode = WorldProbeMode::NoWorld;
};

class GameBridge {
public:
    static RuntimeSnapshot sample();
};

const char* worldProbeModeName(WorldProbeMode mode);
const char* racePlayModeName(RacePlayMode mode);

} // namespace frr::game
