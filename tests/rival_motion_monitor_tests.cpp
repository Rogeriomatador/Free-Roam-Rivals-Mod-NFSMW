#include "domain/RivalMotionMonitor.h"
#include <cstdlib>
#include <iostream>
using namespace frr::domain;
static void test(bool cond,const char* s){if(!cond){std::cerr<<s<<"\n";std::exit(1);}}
int main(){
    RivalMotionMonitor m;
    const EncounterPoint p{0,0,0};
    auto a=m.update(p,0,0.25f,true);
    test(!a.stationaryLong,"first sample no stale evidence");
    for(int i=0;i<48;i++)a=m.update({0.1f*(i%3),0,0},0.1f,0.25f,true);
    test(a.stationaryLong&&a.changed,"12 seconds stationary with jitter detected");
    a=m.update({1,0,0},0.3f,0.25f,true);
    test(a.stationaryLong&&!a.changed,"persistent stationary without spam");
    a=m.update({10,0,0},1.5f,0.25f,true);
    test(!a.stationaryLong&&a.changed,"actual progress clears warning");
    for(int i=0;i<50;i++)a=m.update({10,0,0},0,0.25f,true);
    test(a.stationaryLong,"can detect a second stall");
    a=m.update({10,0,0},0,0.25f,false);
    test(!a.stationaryLong&&a.changed,"world transition revokes stale stall");
    m.reset();test(!m.update(p,0,0.25f,true).stationaryLong,"reset clears");
    for(int i=0;i<60;i++)a=m.update(p,6.0f,0.25f,true);
    test(!a.stationaryLong,"fast rolling always resets");
    std::cout<<"Native racer passive stationary detector and resets passed\n";
}
