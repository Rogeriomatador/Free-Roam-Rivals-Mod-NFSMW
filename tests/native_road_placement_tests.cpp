#include "domain/NativeRoadPlacement.h"
#include <cstdio>
#include <cstdlib>
#include <cmath>
using namespace frr::domain;
void check(bool good,const char* name) {
    if(!good){std::fprintf(stderr,"NativeRoadPlacement failed: %s\n",name);std::exit(1);}
}
VehicleOrientedBox car(float x,float y,float z,float fx=0,float fz=1) {
    VehicleOrientedBox b{};b.valid=true;
    b.center={x,y,z};b.forward={fx,0,fz};
    b.right={fz,0,-fx};b.up={0,1,0};
    b.halfExtents={1.15f,0.75f,2.35f};
    return b;
}
int main() {
    const SpatialVector3 origin{0,0,0},forward{0,0,1};
    auto exact=inspectNativeRoadPlacement(origin,forward,car(0.25f,.8f,.25f));
    check(exact.valid&&!exact.relocated,"original tight gate remains accepted");
    auto offset=inspectNativeRoadPlacement(origin,forward,car(3.35f,.8f,2.02f));
    check(offset.valid&&offset.relocated&&std::abs(offset.horizontalDelta-3.91f)<.04f,
        "v58 observed 3.91-world-unit lane projection gets CONDITIONAL approval");
    auto little=inspectNativeRoadPlacement(origin,forward,car(-1.88f,.9f,-.05f));
    check(little.valid&&little.relocated,"v58 observed 1.88-world-unit lane projection gets conditional approval");
    auto far=inspectNativeRoadPlacement(origin,forward,car(11,0,0));
    check(!far.valid,"arbitrary horizontal teleport rejected");
    auto vertical=inspectNativeRoadPlacement(origin,forward,car(0,5,2));
    check(!vertical.valid,"wrong road deck rejected");
    auto reverse=inspectNativeRoadPlacement(origin,forward,car(3,0,2,0,-1));
    check(!reverse.valid,"opposite lane heading rejected for relocated car");
    auto tooWide=car(0,0,0);tooWide.halfExtents.x=15;
    check(!inspectNativeRoadPlacement(origin,forward,tooWide).valid,"unreserved footprint rejected");
    auto nonfinite=car(1,0,1);nonfinite.center.z=NAN;
    check(!inspectNativeRoadPlacement(origin,forward,nonfinite).valid,"nan physical sample rejected");
    auto bad=car(1,0,1);bad.valid=false;
    check(!inspectNativeRoadPlacement(origin,forward,bad).valid,"invalid native rigid body rejected");
    std::puts("NativeRoadPlacement: original gate, both observed v58 deviations, heading, footprint, height, and NaN verified.");
}
