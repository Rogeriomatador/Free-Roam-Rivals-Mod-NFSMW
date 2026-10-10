#include "domain/RivalLocator.h"
#include <cmath>
#include <limits>
#include <string_view>
int main() {
    using namespace frr::domain;
    const RivalLocatorPoint zero{0,0,0},north{0,0,1};
    const auto a=locateRival(zero,{0,0,50},north,true);
    if(!a.valid||std::abs(a.distanceMeters-50)>0.001f||a.bearing!=RivalBearing::Ahead) return 1;
    if(locateRival(zero,{25,0,0},north,true).bearing!=RivalBearing::Right) return 2;
    if(locateRival(zero,{-25,0,0},north,true).bearing!=RivalBearing::Left) return 3;
    if(locateRival(zero,{0,0,-25},north,true).bearing!=RivalBearing::Behind) return 4;
    if(locateRival(zero,{50,0,50},north,true).bearing!=RivalBearing::AheadRight) return 5;
    if(locateRival(zero,{-50,0,50},north,true).bearing!=RivalBearing::AheadLeft) return 6;
    if(locateRival(zero,{-50,0,-50},north,true).bearing!=RivalBearing::BehindLeft) return 7;
    if(locateRival(zero,{50,0,-50},north,true).bearing!=RivalBearing::BehindRight) return 8;
    if(locateRival(zero,{1,0,0},north,true).bearing!=RivalBearing::Nearby) return 9;
    if(locateRival(zero,{25,0,0},north,false).bearing!=RivalBearing::Unknown) return 10;
    if(locateRival(zero,{25,0,0},{0,0,0},true).bearing!=RivalBearing::Unknown) return 11;
    const auto up=locateRival(zero,{0,12,5},north,true);
    if(!up.valid||std::abs(up.distanceMeters-13)>0.001f) return 12;
    const float nan=std::numeric_limits<float>::quiet_NaN();
    if(locateRival(zero,{nan,0,0},north,true).valid) return 13;
    if(locateRival({nan,0,0},{1,0,0},north,true).valid) return 14;
    if(locateRival(zero,{10,0,0},{nan,0,0},true).bearing!=RivalBearing::Unknown) return 15;
    if(std::string_view(rivalBearingName(RivalBearing::AheadRight))!="FRENTE / DIREITA") return 16;
    return 0;
}
