#include "domain/HudPlacement.h"
#include <iostream>
#include <cstdlib>
#include <cmath>
#include <utility>
#include <initializer_list>
using namespace frr::domain;
static void ok(bool c,const char* why){if(!c){std::cerr<<why<<"\n";std::exit(1);}}
int main(){
    for(const auto [w,h]:{std::pair<float,float>{640,480},{1280,720},{1920,1080},{2560,1080},{3840,2160}}){
        const auto l=placeRivalHud(w,h,2,1);
        ok(l.radarVisible&&l.radarX<w&&l.radarX>0&&l.radarY>0&&l.radarY<h,"right-side radar within active viewport");
        ok(l.routeX<w&&l.routeY<h&&l.scale>0.5,"responsive safe overlays");
        ok(l.radarX>w*0.65f,"right radar not overlapping left native minimap");
        const auto left=placeRivalHud(w,h,0,1);
        ok(left.radarX<w*0.35f&&left.radarY>h*0.65f,"legacy left-bottom option");
        const auto top=placeRivalHud(w,h,1,1);
        ok(top.radarX>w*0.65f&&top.radarY<h*0.40f,"top-right option");
        ok(!placeRivalHud(w,h,3,1).radarVisible,"radar off");
        ok(placeRivalHud(w,h,2,1.5).scale>=l.scale,"scale respects user preference");
    }
    ok(!placeRivalHud(200,100,2,1).radarVisible,"invalid viewport suppresses overlay");
    ok(std::isfinite(placeRivalHud(1280,720,2,NAN).scale),"NaN settings fail closed");
    std::cout<<"HUD safe placements and radar scaling passed\n";
}
