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

struct Vector3Probe {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct RoadGeometryProbe {
    bool available = false;
    bool valid = false;

    std::int16_t segmentIndex = -1;
    int laneIndex = -1;
    float segmentTime = 0.0f;
    float curvature = 0.0f;
    float widthWorldUnits = 0.0f;

    bool deadEnd = false;
    std::int32_t roadOcclusion = 0;
    std::int32_t avoidableOcclusion = 0;
    bool occludedFromBehind = false;

    Vector3Probe position{};
    Vector3Probe forward{};
    Vector3Probe leftPosition{};
    Vector3Probe rightPosition{};
    Vector3Probe startPosition{};
    Vector3Probe endPosition{};
};

struct PlayerRoadNavProbe {
    bool available = false;
    std::uintptr_t vehicleAI = 0;

    Vector3Probe playerPosition{};
    RoadGeometryProbe current{};
    RoadGeometryProbe future{};

    Vector3Probe seekAheadPosition{};
    Vector3Probe farFuturePosition{};
    Vector3Probe farFutureDirection{};

    float seekAheadDistanceWorldUnits = 0.0f;
    float seekAheadProjectionWorldUnits = 0.0f;
    float farFutureDistanceWorldUnits = 0.0f;
    float farFutureProjectionWorldUnits = 0.0f;
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
    bool playerRoadNavigationReadable = false;
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
    PlayerRoadNavProbe roadNav{};
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
