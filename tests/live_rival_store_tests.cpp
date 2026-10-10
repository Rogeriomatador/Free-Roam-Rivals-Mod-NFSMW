#include "persistence/LiveRivalStore.h"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <chrono>
using namespace frr::persistence;
using frr::domain::OutrunOutcome;
void require(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
int main(){
    const auto folder=std::filesystem::temp_directory_path()/ ("frr-live-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
    LiveRivalStore store(folder);LiveRivalRecord r{42};std::string error;
    require(store.load(42).ok&&!store.load(42).found,"missing file starts empty");
    require(!recordLiveResult(r,OutrunOutcome::InProgress),"unfinished races are not stored");
    require(recordLiveResult(r,OutrunOutcome::PlayerWon),"record win");require(store.save(r,error),"save history");
    require(store.load(42).record.wins==1,"reload history");require(store.load(43).record.wins==0,"profile isolation");
    LiveRivalRecord stale{42};require(!store.save(stale,error)&&store.load(42).record.wins==1,"stale writes preserve newer history");
    LiveRivalRecord conflict{42,1,0,1};require(!store.save(conflict,error),"conflicting equal revision rejected");
    const auto text=serializeLiveRival(r);require(parseLiveRival(text).has_value(),"round trip");
    require(!parseLiveRival(text+"junk")&&!parseLiveRival(text.substr(0,text.size()-1)),"trailing or truncated history rejected");
    auto invalid=text;invalid.replace(invalid.find("wins=1"),6,"wins=2");require(!parseLiveRival(invalid),"inconsistent totals rejected");
    {std::ofstream output(store.pathFor(42),std::ios::binary);output<<"CORRUPT ORIGINAL";}
    require(!store.load(42).ok&&!store.save(r,error),"corrupt existing data cannot be overwritten");
    {std::ifstream input(store.pathFor(42));std::string saved;std::getline(input,saved);require(saved=="CORRUPT ORIGINAL","original bytes retained");}
    {std::ofstream output(store.pathFor(43),std::ios::binary);output<<text;}
    require(!store.load(43).ok,"profile mismatch rejected");
    {std::ofstream output(store.pathFor(44),std::ios::binary);output<<std::string(513,'x');}
    require(!store.load(44).ok,"oversized history rejected");
    require(!store.load(0).ok,"missing profile rejected");
    LiveRivalRecord limit{45,1000000,1000000};require(!recordLiveResult(limit,OutrunOutcome::PlayerWon),"bounded counters");
    std::filesystem::remove_all(folder);std::cout<<"Atomic history, corruption and profile tests passed\n";
}
