#pragma once
#include "LiveEncounter.h"
#include <cmath>
namespace frr::domain {
struct FormationCheck {
    bool geometryAligned=false;
    bool bothStopped=false;
    bool worldSafe=false;
    float lateralMeters=0,foreAftMeters=0,distanceMeters=0;
    bool automaticStartAuthorized=false;
};
// Read-only detection before any auto-staging. Control custody, native brake
// and restores are explicitly NOT proven, even when geometry is perfect.
inline FormationCheck checkRaceFormation(
    const EncounterCar& player,const EncounterCar& rival,bool sceneSafe,
    bool playerInputCustodyVerified=false,bool rivalNativeBrakeVerified=false,
    bool fullRestoreVerified=false) {
    FormationCheck o{};
    if(!sceneSafe||!player.valid||!rival.valid)return o;
    for(float v:{player.position.x,player.position.y,player.position.z,
                 rival.position.x,rival.position.y,rival.position.z,
                 player.forward.x,player.forward.y,player.forward.z,
                 rival.forward.x,rival.forward.y,rival.forward.z,
                 player.speed,rival.speed})if(!std::isfinite(v))return o;
    const float pn=std::hypot(player.forward.x,player.forward.z);
    const float rn=std::hypot(rival.forward.x,rival.forward.z);
    if(pn<0.5f||rn<0.5f||pn>1.5f||rn>1.5f)return o;
    const float px=player.forward.x/pn,pz=player.forward.z/pn;
    const float rx=rival.forward.x/rn,rz=rival.forward.z/rn;
    const float dx=rival.position.x-player.position.x;
    const float dz=rival.position.z-player.position.z;
    o.distanceMeters=std::hypot(dx,dz);
    o.foreAftMeters=dx*px+dz*pz;
    o.lateralMeters=std::abs(dx*pz-dz*px);
    o.worldSafe=true;
    o.geometryAligned=std::abs(o.foreAftMeters)<=5.5f &&
        o.lateralMeters>=1.8f&&o.lateralMeters<=7.0f &&
        o.distanceMeters<=8.0f&&
        std::abs(rival.position.y-player.position.y)<=2.0f &&
        px*rx+pz*rz>=0.8f;
    o.bothStopped=player.speed>=0&&rival.speed>=0&&player.speed<1.0f&&rival.speed<1.0f;
    // This gate is intentionally false in v53: there is no validated native
    // player input suppression/restoration or AI braking ABI yet.
    o.automaticStartAuthorized=o.geometryAligned&&o.bothStopped&&
        playerInputCustodyVerified&&rivalNativeBrakeVerified&&fullRestoreVerified;
    return o;
}
} // namespace frr::domain
