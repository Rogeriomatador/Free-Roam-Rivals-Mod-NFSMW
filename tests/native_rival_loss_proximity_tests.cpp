#include "domain/NativeRivalLossProximity.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace frr::domain;
static void check(bool v,const char* m){if(!v){std::cerr<<m<<"\n";std::exit(1);}}
int main() {
    const auto near=classifyNativeRivalLoss(100000,99990,151.178f);
    check(near.observationFresh&&near.disappearedNearby,"actual near loss at 151m");
    check(classifyNativeRivalLoss(100000,99990,249.9f).disappearedNearby,"under threshold");
    check(!classifyNativeRivalLoss(100000,99990,301.0f).disappearedNearby,"distant loss");
    check(classifyNativeRivalLoss(100000,99990,0.0f).disappearedNearby,"zero is within player range");
    check(!classifyNativeRivalLoss(100000,90000,151.178f).observationFresh,"outdated last sighting");
    check(!classifyNativeRivalLoss(100000,100001,151.178f).observationFresh,"clock reversal");
    check(!classifyNativeRivalLoss(100000,0,151.178f).observationFresh,"never seen");
    check(!classifyNativeRivalLoss(100000,99990,-1).observationFresh,"negative distance");
    check(!classifyNativeRivalLoss(100000,99990,std::numeric_limits<float>::quiet_NaN()).observationFresh,"NaN");
    std::cout<<"Native near-range loss is evidence-only and rejects stale telemetry\n";
}
