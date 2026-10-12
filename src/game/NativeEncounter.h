#pragma once
#include "../domain/OutrunRace.h"
#include "../domain/WorldMetricCalibration.h"
namespace frr::game {
struct RuntimeSnapshot;
struct NativeOwnedSnapshot;
struct NativeEncounterConfig {
    bool enabled=false,hudEnabled=true,persistHistory=true;
    bool nativeStreetDuelPilotEnabled=false;
    float streetSprintLengthMeters=120.0f;
    float challengeDistanceMeters=60;
    int radarPosition=2;
    float radarScale=1.0f;
    domain::OutrunTuning tuning{};
};
void configureNativeEncounter(const NativeEncounterConfig& config);
void tickNativeEncounter(const RuntimeSnapshot& world,const NativeOwnedSnapshot& rival,
    const domain::WorldMetricCalibration& metric,float deltaSeconds,bool challengePressed);
void interruptNativeEncounter(const char* reason="world_unsafe");
bool nativeEncounterRunning();
void showNativeRivalStatus(const char* message,const char* detail="");
}
