#include "domain/VisibleSpawnPreference.h"
#include <limits>
#include <iostream>
#include <cstdlib>
using namespace frr::domain;
static void assertTrue(bool v,const char* reason) { if(!v) {std::cerr<<reason<<'\n';std::exit(1);} }
int main() {
    assertTrue(preferDebugSpawnCandidate(true,80,false,20),"potentially onscreen preferred even when farther");
    assertTrue(!preferDebugSpawnCandidate(false,20,true,80),"behind/outside candidate loses");
    assertTrue(preferDebugSpawnCandidate(false,20,false,60),"fallback is near-first");
    assertTrue(preferDebugSpawnCandidate(true,35,true,60),"both potentially visible near-first");
    assertTrue(!preferDebugSpawnCandidate(true,50,true,50),"stable tie");
    assertTrue(!preferDebugSpawnCandidate(false,std::numeric_limits<float>::quiet_NaN(),false,60),"invalid distance loses");
    assertTrue(preferDebugSpawnCandidate(false,60,false,std::numeric_limits<float>::quiet_NaN()),"finite distance beats invalid");
    std::cout<<"Near-player visual priority comparator tests passed\n";
}
