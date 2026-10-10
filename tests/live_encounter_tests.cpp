#include "domain/LiveEncounter.h"
#include "domain/ConfigNumbers.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace frr::domain;
void require(bool ok,const char* message) {if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
LiveEncounterInput input(float player=0,float rival=20) {
    return {{{0,0,player},{0,0,1},10,true},{{0,0,rival},{0,0,1},10,true},0.25f,true,true,false,false};
}
int main() {
    LiveEncounter race({30,1,300,5});auto in=input();
    require(race.tick(in).available,"following permits challenge");
    in.challengeSafe=false;in.acceptPressed=true;
    require(!race.tick(in).running,"police blocks acceptance");
    in.challengeSafe=true;auto v=race.tick(in);require(v.justStarted&&v.running,"accepted challenge starts");
    in.acceptPressed=false;in.challengeSafe=false; // Already accepted race continues in pursuit.
    for(int n=1;n<=8;++n) {in.player.position.z=n*2.5f;in.rival.position.z=20+n*5.0f;in.rival.speed=20;v=race.tick(in);if(v.justFinished)break;}
    require(v.justFinished&&v.outcome==OutrunOutcome::RivalWon,"same trail lead and hold produce loss during pursuit");
    require(!race.tick(in).justFinished,"result is emitted once");
    in.acceptPressed=true;in.challengeSafe=true;require(!race.tick(in).running,"cooldown blocks immediate repeat");
    LiveEncounter overtake({20,0.5f,300,5});in=input();in.acceptPressed=true;overtake.tick(in);
    in.acceptPressed=false;in.player.speed=40;
    for(int n=1;n<=8;++n) {in.player.position.z=n*10.0f;in.rival.position.z=20+n*2.5f;v=overtake.tick(in);if(v.justFinished)break;}
    require(v.outcome==OutrunOutcome::PlayerWon,"verified nearby overtake then player trail wins");
    LiveEncounter wrongRoad;in=input();in.acceptPressed=true;wrongRoad.tick(in);in.acceptPressed=false;
    for(int n=1;n<=16;++n) {in.player.position.x=std::min(n*5.0f,35.0f);in.player.position.z=n*2.5f;in.rival.position.z=20+n*2.5f;v=wrongRoad.tick(in);if(v.justFinished)break;}
    require(v.outcome==OutrunOutcome::Aborted,"different street cannot award a win");
    LiveEncounter bridge;in=input();in.acceptPressed=true;bridge.tick(in);in.acceptPressed=false;
    for(int n=1;n<=20;++n) {in.player.position.y=std::min(n*3.0f,15.0f);in.player.position.z=n*2.5f;in.rival.position.z=20+n*2.5f;v=bridge.tick(in);if(v.justFinished)break;}
    require(v.outcome==OutrunOutcome::Aborted,"overpass cannot score from horizontal proximity");
    LiveEncounter teleport;in=input();in.acceptPressed=true;teleport.tick(in);in.player.position.z=1000;
    require(teleport.tick(in).outcome==OutrunOutcome::Aborted,"teleport cancels");
    LiveEncounter transition;in=input();in.acceptPressed=true;transition.tick(in);in.worldSafe=false;
    require(transition.tick(in).justFinished,"world transition interrupts");require(!transition.interrupt().justFinished,"repeated interrupt is idempotent");
    LiveEncounter paused;in=input();in.acceptPressed=true;paused.tick(in);in.deltaSeconds=0;in.player.position.z=900;
    require(paused.tick(in).running,"zero delta freezes scoring");in.deltaSeconds=0.25f;
    require(paused.tick(in).outcome==OutrunOutcome::Aborted,"resume rechecks continuity");
    LiveEncounter corner;in=input();in.acceptPressed=true;corner.tick(in);in.acceptPressed=false;
    for(int n=1;n<=16;++n) {
        in.rival.position={n<=8?0.0f:(n-8)*2.5f,0,n<=8?20+n*2.5f:40.0f};
        in.rival.forward=n<=8?EncounterPoint{0,0,1}:EncounterPoint{1,0,0};
        in.player.position={0,0,n*2.5f};v=corner.tick(in);
    }
    require(v.running&&v.routeMatched,"observed trail matches follower before same corner");
    in.player.position.x=std::numeric_limits<float>::quiet_NaN();
    require(corner.tick(in).outcome==OutrunOutcome::Aborted,"nonfinite geometry rejected");
    require(finiteConfigFloat("nan",7)==7&&finiteConfigFloat("inf",7)==7&&finiteConfigFloat("2junk",7)==7,"invalid float settings fall back");
    require(finiteConfigFloat(" 2.5 \t",7)==2.5f&&finiteConfigFloat("1e99",7)==7,"finite settings and overflow");
    require(finiteConfigUnsigned("0x47",1)==71&&finiteConfigUnsigned("0",1)==0,"hex input key and disabled value");
    require(finiteConfigUnsigned("-1",7)==7&&finiteConfigUnsigned("9999999999999999999",7)==7&&finiteConfigUnsigned("1x",7)==7,"invalid unsigned settings rejected");
    std::cout<<"Live encounter route, pursuit, overtake, lifecycle and config tests passed\n";
}
