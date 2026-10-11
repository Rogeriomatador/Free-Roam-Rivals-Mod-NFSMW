#pragma once
#include <cmath>

namespace frr::domain {
// Debug-spawn ranking only. A camera frustum intersection means merely
// "potentially in the primary view", never an engine rendering guarantee.
inline bool preferDebugSpawnCandidate(bool aPotentiallyOnscreen,float aDistance,
    bool bPotentiallyOnscreen,float bDistance) {
    if(aPotentiallyOnscreen!=bPotentiallyOnscreen) return aPotentiallyOnscreen;
    const bool aValid=std::isfinite(aDistance)&&aDistance>=0;
    const bool bValid=std::isfinite(bDistance)&&bDistance>=0;
    if(aValid!=bValid) return aValid;
    if(!aValid) return false;
    return aDistance<bDistance;
}
} // namespace frr::domain
