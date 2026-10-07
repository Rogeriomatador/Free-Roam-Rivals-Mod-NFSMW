#include "RuntimeEvidence.h"

namespace frr::domain {
bool runtimeEvidenceUsable(
    const RuntimeEvidenceStamp& captured,
    const RuntimeEvidenceStamp& current,
    std::uint64_t nowMillis,
    std::uint64_t maximumAgeMillis
) {
    return captured.safeFreeRoam && current.safeFreeRoam &&
        captured.generation != 0 && captured.generation == current.generation &&
        captured.playerIVehicle != 0 && captured.playerIVehicle == current.playerIVehicle &&
        captured.playerPVehicle != 0 && captured.playerPVehicle == current.playerPVehicle &&
        captured.roadNetwork != 0 && captured.roadNetwork == current.roadNetwork &&
        captured.raceStatus != 0 && captured.raceStatus == current.raceStatus &&
        captured.profileKey != 0 && captured.profileKey == current.profileKey &&
        nowMillis >= captured.capturedAtMillis &&
        nowMillis - captured.capturedAtMillis <= maximumAgeMillis;
}
} // namespace frr::domain
