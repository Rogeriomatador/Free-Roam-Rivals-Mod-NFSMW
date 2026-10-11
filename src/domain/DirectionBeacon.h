#pragma once
#include "LiveEncounter.h"
#include <cmath>
#include <initializer_list>

namespace frr::domain {
// View-relative direction derived from verified 3-D world coordinates and
// the player's physical orientation. Not a road graph or native minimap node.
struct DirectionBeacon {
    bool valid=false;
    float front=0,right=0,distanceMeters=0;
};
inline DirectionBeacon directionBeacon(EncounterPoint player,EncounterPoint facing,
    EncounterPoint target) {
    DirectionBeacon out{};
    for(float x:{player.x,player.y,player.z,facing.x,facing.y,facing.z,
                 target.x,target.y,target.z}) if(!std::isfinite(x)) return out;
    const float f=std::hypot(facing.x,facing.z);
    const float dx=target.x-player.x,dz=target.z-player.z;
    const float m=std::hypot(dx,dz);
    if(!std::isfinite(f)||f<0.5f||f>1.5f||!std::isfinite(m)||m<0.1f||m>3000)
        return out;
    out.distanceMeters=std::hypot(m,target.y-player.y);
    if(!std::isfinite(out.distanceMeters)) return {};
    out.front=(dx*facing.x+dz*facing.z)/(f*m);
    out.right=(dx*facing.z-dz*facing.x)/(f*m);
    out.valid=std::isfinite(out.front)&&std::isfinite(out.right);
    return out;
}
}
