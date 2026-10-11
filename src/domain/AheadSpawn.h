#pragma once
#include "VehicleSpatialEvidence.h"
#include <cmath>
#include <algorithm>

namespace frr::domain {

// A *position* ahead of the player's verified rigid-body orientation, not
// the motion/velocity vector (player may be stopped or reversing).
// Never authorizes native construction, rendering or streaming.
struct AheadSpawnEvaluation {
    bool ahead=false;
    float forwardMeters=0.0f, lateralMeters=0.0f, heightMeters=0.0f;
    bool alignedWithPlayer=false;
};
inline AheadSpawnEvaluation evaluateAheadSpawn(
    SpatialVector3 playerPosition,SpatialVector3 playerForward,
    SpatialVector3 targetPosition,SpatialVector3 targetForward,
    float worldUnitsPerMeter) {
    AheadSpawnEvaluation result{};
    if(!(std::isfinite(worldUnitsPerMeter)&&worldUnitsPerMeter>=0.1f&&
         worldUnitsPerMeter<=10.0f)) return result;
    for(float number:{playerPosition.x,playerPosition.y,playerPosition.z,
                      playerForward.x,playerForward.y,playerForward.z,
                      targetPosition.x,targetPosition.y,targetPosition.z,
                      targetForward.x,targetForward.y,targetForward.z})
        if(!std::isfinite(number)) return result;
    const float norm=std::hypot(playerForward.x,playerForward.z);
    if(!(std::isfinite(norm)&&norm>0.5f&&norm<1.5f)) return result;
    const float fx=playerForward.x/norm, fz=playerForward.z/norm;
    const float dx=(targetPosition.x-playerPosition.x)/worldUnitsPerMeter;
    const float dy=(targetPosition.y-playerPosition.y)/worldUnitsPerMeter;
    const float dz=(targetPosition.z-playerPosition.z)/worldUnitsPerMeter;
    result.forwardMeters=dx*fx+dz*fz;
    result.lateralMeters=std::abs(dx*fz-dz*fx);
    result.heightMeters=std::abs(dy);
    const float roadNorm=std::hypot(targetForward.x,targetForward.z);
    if(std::isfinite(roadNorm)&&roadNorm>0.5f)
        result.alignedWithPlayer=(targetForward.x*fx+targetForward.z*fz)/roadNorm>0.3f;
    // Stay in a forward cone, roughly 20-115 metres ahead, excluding
    // parallel roads 30+ metres aside and high/low grade-separated bridges.
    const float maxLateral=std::min(30.0f,std::max(10.0f,0.5f*result.forwardMeters));
    result.ahead=std::isfinite(result.forwardMeters)&&
        std::isfinite(result.lateralMeters)&&std::isfinite(result.heightMeters)&&
        result.forwardMeters>=20.0f&&result.forwardMeters<=115.0f&&
        result.lateralMeters<=maxLateral&&result.heightMeters<=7.5f;
    return result;
}

inline bool preferAheadSpawn(const AheadSpawnEvaluation& a,bool aPotentiallyOnscreen,
                             const AheadSpawnEvaluation& b,bool bPotentiallyOnscreen) {
    if(a.ahead!=b.ahead) return a.ahead;
    if(aPotentiallyOnscreen!=bPotentiallyOnscreen) return aPotentiallyOnscreen;
    if(a.alignedWithPlayer!=b.alignedWithPlayer) return a.alignedWithPlayer;
    // Distances near 45 metres help players spot the car before native Racer
    // AI drives away. Prefer centred lane of travel within safe forward cone.
    const float aScore=std::abs(a.forwardMeters-45.0f)+0.8f*a.lateralMeters;
    const float bScore=std::abs(b.forwardMeters-45.0f)+0.8f*b.lateralMeters;
    return aScore<bScore;
}
} // namespace frr::domain
