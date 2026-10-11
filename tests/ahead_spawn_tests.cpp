#include "domain/AheadSpawn.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace frr::domain;
static void require(bool x,const char* t){if(!x){std::cerr<<t<<"\n";std::exit(1);}}
static SpatialVector3 P(float x,float y,float z){return {x,y,z};}
int main(){
    const auto facing=P(0,0,1);
    const auto a=evaluateAheadSpawn(P(0,0,0),facing,P(0,0,40),facing,1);
    require(a.ahead&&a.forwardMeters==40&&a.lateralMeters==0&&a.alignedWithPlayer,
        "40m straight ahead accepted");
    require(!evaluateAheadSpawn(P(0,0,0),facing,P(0,0,-40),facing,1).ahead,
        "behind player rejected even if camera points backward");
    require(!evaluateAheadSpawn(P(0,0,0),facing,P(0,0,5),facing,1).ahead,
        "too close rejected");
    require(!evaluateAheadSpawn(P(0,0,0),facing,P(0,0,140),facing,1).ahead,
        "too far rejected");
    require(!evaluateAheadSpawn(P(0,0,0),facing,P(50,0,50),facing,1).ahead,
        "parallel road too far to the side rejected");
    require(!evaluateAheadSpawn(P(0,0,0),facing,P(0,12,50),facing,1).ahead,
        "other deck of a bridge rejected");
    require(!evaluateAheadSpawn(P(0,0,0),P(0,0,0),P(0,0,50),facing,1).ahead,
        "unverified player heading rejected");
    require(!evaluateAheadSpawn(P(0,0,0),facing,P(0,0,50),facing,0).ahead,
        "unverified calibration rejected");
    require(!evaluateAheadSpawn(P(0,0,0),facing,P(0,0,50),facing,
        std::numeric_limits<float>::quiet_NaN()).ahead,"NaN scale rejected");
    const auto scaled=evaluateAheadSpawn(P(0,0,0),P(1,0,0),P(75,0,0),P(1,0,0),1.5f);
    require(scaled.ahead&&scaled.forwardMeters==50,"world unit scale applied");
    const auto backing=evaluateAheadSpawn(P(0,0,0),facing,P(0,0,45),P(0,0,-1),1);
    require(backing.ahead&&!backing.alignedWithPlayer,
        "opposing direction is an ahead road but ranked lower");
    require(preferAheadSpawn(a,true,backing,false),"visible aligned road wins");
    require(!preferAheadSpawn(backing,true,a,true),"same-visibility aligned road wins");
    const auto side=evaluateAheadSpawn(P(0,0,0),facing,P(17,0,45),facing,1);
    require(side.ahead&&preferAheadSpawn(a,true,side,true),
        "within the front corridor the better aligned lane wins");
    require(preferAheadSpawn(a,false,backing,true)==false,
        "camera intersecting candidate receives priority after ahead gate");
    std::cout<<"Player forward spawn cone, registry orientation, safety limits and preference passed\n";
}
