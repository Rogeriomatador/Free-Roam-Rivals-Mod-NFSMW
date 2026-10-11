#include "domain/DirectionBeacon.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace frr::domain;
static void ok(bool x,const char* why){if(!x){std::cerr<<why<<"\n";std::exit(1);}}
int main(){
    const EncounterPoint at{0,0,0},ahead{0,0,1};
    auto b=directionBeacon(at,ahead,{0,0,20});
    ok(b.valid&&std::abs(b.front-1)<0.001f&&b.distanceMeters==20,"straight arrow up");
    b=directionBeacon(at,ahead,{20,0,0});
    ok(b.valid&&std::abs(b.right-1)<0.001f,"right arrow");
    b=directionBeacon(at,ahead,{-20,0,0});
    ok(b.valid&&std::abs(b.right+1)<0.001f,"left arrow");
    b=directionBeacon(at,ahead,{0,0,-20});
    ok(b.valid&&b.front< -0.99f,"behind arrow down");
    b=directionBeacon(at,{0,0,0},{0,0,20});
    ok(!b.valid,"no player orientation means no radar data");
    b=directionBeacon(at,ahead,{0,0,0});
    ok(!b.valid,"no zero-distance arrow");
    b=directionBeacon(at,ahead,{std::numeric_limits<float>::quiet_NaN(),0,1});
    ok(!b.valid,"reject NaNs");
    std::cout<<"Verified orientation-only green beacon and route cue directions passed\n";
}
