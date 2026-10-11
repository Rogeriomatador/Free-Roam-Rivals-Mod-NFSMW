#include "domain/ActiveModelObservation.h"
#include <cstdlib>
#include <iostream>
#include <utility>
using namespace frr::domain;
static void test(bool ok,const char* msg) {if(!ok){std::cerr<<msg<<'\n';std::exit(1);}}
int main(){
    VehicleOrientedBox player{};player.valid=true;player.identity=1;player.vehicleKey=100;
    VehicleOrientedBox rival{};rival.valid=true;rival.identity=2;rival.vehicleKey=100;
    VehicleOrientedBox unrelated{};unrelated.valid=true;unrelated.identity=3;unrelated.vehicleKey=200;
    VehicleOrientedBox invalid{};invalid.identity=4;invalid.vehicleKey=100;
    const std::vector<VehicleOrientedBox> fleet{player,rival,unrelated,invalid};
    auto x=observeActiveModel(fleet,true,0,100,1);
    test(x.reliable&&x.observed==2&&x.otherThanPlayer==1,"count active player plus other same-key vehicle");
    x=observeActiveModel(fleet,true,0,200,1);
    test(x.reliable&&x.observed==1&&x.otherThanPlayer==1,"different model counted correctly");
    x=observeActiveModel(fleet,true,0,300,1);
    test(x.reliable&&x.observed==0,"missing model is only absent in current live snapshot");
    for(auto [complete,fail]:{std::pair<bool,unsigned>{false,0},{true,1}}){
        x=observeActiveModel(fleet,complete,fail,100,1);
        test(!x.reliable&&x.observed==0&&x.otherThanPlayer==0,"fail closed on unknown fleet");
    }
    x=observeActiveModel(fleet,true,0,100,0);
    test(!x.reliable&&x.observed==0,"missing player identity fails closed");
    x=observeActiveModel(fleet,true,0,0,1);
    test(!x.reliable&&x.observed==0,"zero model hash invalid");
    std::cout<<"Live-model evidence bounds, non-player classification and failures passed\n";
}
