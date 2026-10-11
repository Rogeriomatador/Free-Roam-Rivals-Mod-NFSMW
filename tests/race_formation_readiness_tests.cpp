#include "domain/RaceFormationReadiness.h"
#include <iostream>
#include <cstdlib>
#include <limits>
using namespace frr::domain;
static void require(bool yes,const char* msg){if(!yes){std::cerr<<msg<<"\n";std::exit(1);}}
static EncounterCar car(float x,float z,float fx=0,float fz=1,float speed=0){
    return {{x,0,z},{fx,0,fz},speed,true};
}
int main(){
    const auto side=checkRaceFormation(car(0,0),car(3.6f,0),true);
    require(side.geometryAligned&&side.bothStopped&&side.worldSafe,"valid side-by-side start");
    require(!side.automaticStartAuthorized,"geometry cannot authorize automatic input lock");
    require(!checkRaceFormation(car(0,0),car(3.6f,0),true,true,false,true).automaticStartAuthorized,
        "AI brake ABI must be proven");
    require(!checkRaceFormation(car(0,0),car(3.6f,0),true,true,true,false).automaticStartAuthorized,
        "input restore must be proven");
    require(checkRaceFormation(car(0,0),car(3.6f,0),true,true,true,true).automaticStartAuthorized,
        "all future authorization conditions required");
    require(!checkRaceFormation(car(0,0),car(3.6f,0),false,true,true,true).automaticStartAuthorized,
        "world transition denies start");
    require(!checkRaceFormation(car(0,0),car(0,30),true).geometryAligned,
        "not a line-up when one car too far ahead");
    require(!checkRaceFormation(car(0,0),car(3.6f,0,0,-1),true).geometryAligned,
        "opposite road direction");
    require(!checkRaceFormation(car(0,0),car(3.6f,0,0,1,4),true).bothStopped,
        "not ready to count down when rival accelerating");
    auto bad=car(0,0);bad.position.x=std::numeric_limits<float>::quiet_NaN();
    require(!checkRaceFormation(bad,car(3.6f,0),true).geometryAligned,
        "nonfinite physical data rejected");
    std::cout<<"Staging side-by-side read-only readiness and custody gates passed\n";
}
