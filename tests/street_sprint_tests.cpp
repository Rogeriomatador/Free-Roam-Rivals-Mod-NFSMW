#include "domain/StreetSprint.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace frr::domain;
void require(bool yes,const char* why) {
    if(!yes){std::cerr<<"StreetSprint FAILED: "<<why<<"\n";std::exit(1);}
}
EncounterCar car(float x,float z,float speed=0) {
    EncounterCar c{};
    c.position={x,0,z};c.forward={0,0,1};c.speed=speed;c.valid=true;
    return c;
}
StreetSprintFrame frame(float p,float r,float ps=100,float rs=80,float dt=.1f){
    return {car(-3,p,ps),car(3,r,rs),dt,true,false};
}
int main(){
    {
        StreetSprint sprint;
        require(!sprint.begin(car(0,0),car(1,0)),"cannot start without physical side-by-side separation");
        require(sprint.begin(car(-3,0),car(3,0)),"real aligned standing start");
        require(std::abs(sprint.view().finishPoint.z-120)<.01f,"shared finish coordinate");
        require(sprint.running(),"active after start");
        StreetSprintView result{};
        for(int i=1;i<=12;++i) result=sprint.tick(frame(10.0f*i,8.0f*i));
        require(result.justFinished && result.outcome==OutrunOutcome::PlayerWon,"player won crossing first");
        require(!sprint.running(),"one result recorded only");
        require(!sprint.tick(frame(130,104)).justFinished,"no duplicate result");
    }
    {
        StreetSprint sprint;require(sprint.begin(car(-3,0),car(3,0)),"second start");
        for(int i=1;i<=11;++i)require(sprint.tick(frame(8.0f*i,10.0f*i,80,100)).running,"race continues until cross");
        auto v=sprint.tick(frame(96,120,80,100));
        require(v.outcome==OutrunOutcome::RivalWon,"rival wins independently");
    }
    {
        StreetSprint sprint;require(sprint.begin(car(-3,0),car(3,0)),"draw start");
        for(int i=1;i<12;++i)sprint.tick(frame(10.0f*i,10.0f*i,100,100));
        auto v=sprint.tick(frame(120,120,100,100));
        require(v.outcome==OutrunOutcome::Draw && v.justFinished,"equal crossing is true draw");
    }
    {
        StreetSprint sprint;require(sprint.begin(car(-3,0),car(3,0)),"teleport start");
        auto v=sprint.tick(frame(120,0,0,0));
        require(v.outcome==OutrunOutcome::Aborted && !v.running,"teleport cannot win");
    }
    {
        StreetSprint sprint;require(sprint.begin(car(-3,0),car(3,0)),"shortcut start");
        auto v=sprint.tick({car(30,15,100),car(3,15,100),.15f,true,false});
        require(v.outcome==OutrunOutcome::Aborted,"off-axis lateral corridor rejected");
    }
    {
        StreetSprint sprint;require(sprint.begin(car(-3,0),car(3,0)),"cancel start");
        auto v=sprint.tick({car(-3,0),car(3,0),.016f,true,true});
        require(v.outcome==OutrunOutcome::Aborted,"cancel is not loss");
    }
    {
        StreetSprint sprint;require(sprint.begin(car(-3,0),car(3,0)),"loss start");
        auto v=sprint.tick({car(-3,0),car(3,0),.016f,false,false});
        require(v.outcome==OutrunOutcome::Aborted,"native rival disappears during sprint");
    }
    {
        StreetSprint sprint;require(sprint.begin(car(-3,0),car(3,0)),"time start");
        auto v=sprint.tick({car(-3,0),car(3,0),std::numeric_limits<float>::quiet_NaN(),true,false});
        require(v.outcome==OutrunOutcome::Aborted,"invalid time cannot award win");
    }
    std::cout<<"StreetSprint shared physical finish, win/loss/draw, anti-teleport and abort verified\n";
}
