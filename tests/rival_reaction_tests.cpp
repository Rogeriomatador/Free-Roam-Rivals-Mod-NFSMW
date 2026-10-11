#include "domain/RivalReaction.h"
#include <cmath>
#include <limits>
#include <string_view>
int main() {
    using namespace frr::domain;
    RivalReactionMonitor monitor;
    RivalReactionInput s{};
    s.telemetrySafe=true;s.elapsedSeconds=0.25f;
    s.player={{0,0,0},{0,0,1},20,true};
    s.rival={{0,0,250},{0,0,1},20,true};
    auto frame=[&](int n) {RivalReactionView v{};for(int i=0;i<n;++i)v=monitor.update(s);return v;};
    if(frame(3).state!=RivalReaction::Distant) return 1;
    s.rival.position.z=50;
    if(frame(1).state!=RivalReaction::Distant) return 2; // hysteresis
    if(frame(2).state!=RivalReaction::Following) return 3;
    s.challengeAvailable=true;
    if(frame(3).state!=RivalReaction::Ready) return 4;
    s.cooldownSeconds=10;s.challengeAvailable=false;
    if(frame(3).state!=RivalReaction::Cooldown) return 5;
    s.raceRunning=true;
    if(frame(1).state!=RivalReaction::Racing) return 6;
    s.raceRunning=false;s.cooldownSeconds=0;
    s.player.position.y=9;
    if(frame(3).state!=RivalReaction::DifferentLevel) return 7;
    s.telemetrySafe=false;
    const auto invalid=frame(1);
    if(invalid.state!=RivalReaction::Unavailable||invalid.distanceValid==true) return 8;
    s.telemetrySafe=true;s.player.position.y=0;s.player.valid=false;
    s.rival.position.z=130;
    if(frame(3).state!=RivalReaction::Cruising) return 9;
    // Legitimate closing gaps, even while facing differently, use actual positions.
    for(int z=3;z<=27;z+=3) {s.player.position.z=static_cast<float>(z);monitor.update(s);}
    if(frame(1).state!=RivalReaction::Closing) return 10;
    for(int z=24;z>=-3;z-=3) {s.player.position.z=static_cast<float>(z);monitor.update(s);}
    if(frame(1).state!=RivalReaction::FallingBehind) return 11;
    s.player.position.z=5000;
    if(frame(3).state!=RivalReaction::Distant) return 12; // no false close trend on teleport
    s.rival.position.y=std::numeric_limits<float>::quiet_NaN();
    if(frame(1).state!=RivalReaction::Unavailable) return 13;
    s.rival.position={0,0,50};s.player.position={0,0,0};s.elapsedSeconds=0;s.telemetrySafe=true;
    if(frame(2).state!=RivalReaction::Unavailable) return 14; // frozen physics never advances hysteresis
    if(std::string_view(rivalReactionLabel(RivalReaction::Ready))!="RICO AO ALCANCE - DESAFIE") return 15;
    return 0;
}
