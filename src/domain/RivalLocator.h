#pragma once
#include <cmath>
namespace frr::domain {
struct RivalLocatorPoint {float x=0,y=0,z=0;};
enum class RivalBearing {Unknown,Nearby,Ahead,Behind,Left,Right,AheadLeft,AheadRight,BehindLeft,BehindRight};
struct RivalLocatorFix {bool valid=false;float distanceMeters=0;RivalBearing bearing=RivalBearing::Unknown;};
inline bool finiteLocatorPoint(RivalLocatorPoint p) {
    return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);
}
// Straight-line 3D metric distance; relative heading comes from player's actual motion.
inline RivalLocatorFix locateRival(RivalLocatorPoint player,RivalLocatorPoint rival,RivalLocatorPoint direction,bool moving) {
    RivalLocatorFix fix{};
    if(!finiteLocatorPoint(player)||!finiteLocatorPoint(rival)) return fix;
    const float dx=rival.x-player.x,dy=rival.y-player.y,dz=rival.z-player.z;
    const float horizontal=std::hypot(dx,dz),total=std::hypot(horizontal,dy);
    if(!std::isfinite(total)) return fix;
    fix.valid=true;fix.distanceMeters=total;
    if(!moving||!finiteLocatorPoint(direction)||std::hypot(direction.x,direction.z)<0.001f) return fix;
    if(horizontal<3.0f) {fix.bearing=RivalBearing::Nearby;return fix;}
    const float ahead=dx*direction.x+dz*direction.z;
    const float right=dx*direction.z-dz*direction.x;
    if(std::abs(ahead)>=1.5f*std::abs(right)) fix.bearing=ahead>=0?RivalBearing::Ahead:RivalBearing::Behind;
    else if(std::abs(right)>=1.5f*std::abs(ahead)) fix.bearing=right>=0?RivalBearing::Right:RivalBearing::Left;
    else if(ahead>=0) fix.bearing=right>=0?RivalBearing::AheadRight:RivalBearing::AheadLeft;
    else fix.bearing=right>=0?RivalBearing::BehindRight:RivalBearing::BehindLeft;
    return fix;
}
inline const char* rivalBearingName(RivalBearing b) {
    switch(b) {
        case RivalBearing::Nearby:return "PROXIMO";
        case RivalBearing::Ahead:return "FRENTE";
        case RivalBearing::Behind:return "ATRAS";
        case RivalBearing::Left:return "ESQUERDA";
        case RivalBearing::Right:return "DIREITA";
        case RivalBearing::AheadLeft:return "FRENTE / ESQUERDA";
        case RivalBearing::AheadRight:return "FRENTE / DIREITA";
        case RivalBearing::BehindLeft:return "ATRAS / ESQUERDA";
        case RivalBearing::BehindRight:return "ATRAS / DIREITA";
        default:return "DESCONHECIDA";
    }
}
}
