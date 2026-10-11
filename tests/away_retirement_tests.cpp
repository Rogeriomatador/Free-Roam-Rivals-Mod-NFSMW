#include "domain/AwayRetirement.h"
#include <limits>
#include <cstdlib>
#include <iostream>
using namespace frr::domain;
static void check(bool yes,const char* why) {
    if(!yes){std::cerr<<why<<'\n';std::exit(1);}
}
int main() {
    AwayRetirementInput in{true,true,true,true,true,false,6,410.0f};
    check(shouldProbeSafeAwayRetirement(in),"stable near prototype may probe hidden/occlusion");
    in.nearPlayerPrototype=false;check(!shouldProbeSafeAwayRetirement(in),"no automatic retirement of long-distance release preset");
    in.nearPlayerPrototype=true;in.nativeCarActive=false;check(!shouldProbeSafeAwayRetirement(in),"only native-active vehicle");
    in.nativeCarActive=true;in.loaded=false;check(!shouldProbeSafeAwayRetirement(in),"never probe already streamed out vehicle");
    in.loaded=true;in.validBox=false;check(!shouldProbeSafeAwayRetirement(in),"invalid box no native retirement");
    in.validBox=true;in.worldAndPursuitClear=false;check(!shouldProbeSafeAwayRetirement(in),"never retire during pursuit/transition");
    in.worldAndPursuitClear=true;in.contestRunning=true;check(!shouldProbeSafeAwayRetirement(in),"cannot retire active race");
    in.contestRunning=false;in.stableSamples=5;check(!shouldProbeSafeAwayRetirement(in),"require sustained safe world");
    in.stableSamples=6;in.verifiedDistanceMeters=340;check(!shouldProbeSafeAwayRetirement(in),"do not retire visible near rival");
    in.verifiedDistanceMeters=1000;check(!shouldProbeSafeAwayRetirement(in),"avoid native reads after likely streaming unload");
    in.verifiedDistanceMeters=std::numeric_limits<float>::quiet_NaN();
    check(!shouldProbeSafeAwayRetirement(in),"unknown distance not trusted");
    std::cout<<"Safe offscreen retirement gate tests passed\n";
}
