#pragma once
#include "LiveEncounter.h"
#include <algorithm>
#include <cmath>

namespace frr::domain {

// This monitor interprets VERIFIED measured positions and speeds only.
// It does not drive, accelerate, steer or alter the native rival.
enum class RivalReaction {
    Unavailable, Distant, DifferentLevel, Cruising, Closing, FallingBehind,
    Following, Ready, Cooldown, Racing
};
struct RivalReactionInput {
    EncounterCar player{},rival{};
    float elapsedSeconds=0;
    bool telemetrySafe=false,raceRunning=false,challengeAvailable=false;
    float cooldownSeconds=0;
};
struct RivalReactionView {
    RivalReaction state=RivalReaction::Unavailable;
    float distanceMeters=0;
    bool distanceValid=false,changed=false;
};
inline const char* rivalReactionLabel(RivalReaction state) {
    switch(state) {
        case RivalReaction::Distant:return "RICO ESTA LONGE";
        case RivalReaction::DifferentLevel:return "RICO EM OUTRO NIVEL";
        case RivalReaction::Cruising:return "RICO CIRCULANDO";
        case RivalReaction::Closing:return "VOCE SE APROXIMA DE RICO";
        case RivalReaction::FallingBehind:return "RICO SE AFASTA";
        case RivalReaction::Following:return "VOCE ESTA SEGUINDO RICO";
        case RivalReaction::Ready:return "RICO AO ALCANCE - DESAFIE";
        case RivalReaction::Cooldown:return "RICO AGUARDANDO NOVO DESAFIO";
        case RivalReaction::Racing:return "DESAFIO EM ANDAMENTO";
        default:return "RIVAL SEM TELEMETRIA CONFIAVEL";
    }
}
class RivalReactionMonitor {
    RivalReaction current_=RivalReaction::Unavailable,proposed_=RivalReaction::Unavailable;
    float pendingSeconds_=0,previousDistance_=0,trendSpeed_=0;
    bool haveDistance_=false,haveTrend_=false;
    static float distance(EncounterPoint a,EncounterPoint b) {
        return std::hypot(std::hypot(a.x-b.x,a.z-b.z),a.y-b.y);
    }
    static bool finite(EncounterPoint p) {
        return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);
    }
public:
    void reset() {
        current_=proposed_=RivalReaction::Unavailable;pendingSeconds_=previousDistance_=trendSpeed_=0;
        haveDistance_=haveTrend_=false;
    }
    RivalReactionView update(const RivalReactionInput& in) {
        RivalReactionView out{};
        if(!in.telemetrySafe||!finite(in.player.position)||!finite(in.rival.position)||
           !std::isfinite(in.player.speed)||!std::isfinite(in.rival.speed)||
           in.player.speed<0||in.rival.speed<0||in.player.speed>150||in.rival.speed>150||
           !std::isfinite(in.elapsedSeconds)||in.elapsedSeconds<0||in.elapsedSeconds>1||
           !std::isfinite(in.cooldownSeconds)||in.cooldownSeconds<0) {
            const bool changed=current_!=RivalReaction::Unavailable;
            reset();out.changed=changed;return out;
        }
        const float d=distance(in.player.position,in.rival.position);
        if(!std::isfinite(d)) {const bool changed=current_!=RivalReaction::Unavailable;reset();out.changed=changed;return out;}
        out.distanceMeters=d;out.distanceValid=true;
        if(haveDistance_ && in.elapsedSeconds>0.01f) {
            const float delta=previousDistance_-d;
            // Never classify teleport/discontinuity or zero-delta snapshots as a chase.
            const float maximum=(in.player.speed+in.rival.speed+15)*in.elapsedSeconds+5;
            if(std::abs(delta)<=maximum) {
                const float observed=delta/in.elapsedSeconds;
                trendSpeed_=haveTrend_?0.65f*trendSpeed_+0.35f*observed:observed;
                haveTrend_=true;
            } else {trendSpeed_=0;haveTrend_=false;}
        } else if(!haveDistance_) {trendSpeed_=0;haveTrend_=false;}
        previousDistance_=d;haveDistance_=true;
        RivalReaction next=RivalReaction::Cruising;
        if(in.raceRunning) next=RivalReaction::Racing;
        else if(d>200) next=RivalReaction::Distant;
        else if(std::abs(in.player.position.y-in.rival.position.y)>4) next=RivalReaction::DifferentLevel;
        else if(in.cooldownSeconds>0) next=RivalReaction::Cooldown;
        else if(in.challengeAvailable) next=RivalReaction::Ready;
        else if(d<=65&&in.player.valid&&in.rival.valid&&in.player.speed>=3&&in.rival.speed>=3) {
            const auto deltaX=in.player.position.x-in.rival.position.x;
            const auto deltaZ=in.player.position.z-in.rival.position.z;
            const float behind=deltaX*in.rival.forward.x+deltaZ*in.rival.forward.z;
            const float parallel=in.player.forward.x*in.rival.forward.x+in.player.forward.z*in.rival.forward.z;
            const float lateral=std::abs(deltaX*in.rival.forward.z-deltaZ*in.rival.forward.x);
            // Do not claim following while the cars are on distant parallel streets.
            if(behind< -1 && lateral<=7.0f && parallel>0.75f) next=RivalReaction::Following;
        }
        if(next==RivalReaction::Cruising&&d<=200&&haveTrend_) {
            if(trendSpeed_>=4) next=RivalReaction::Closing;
            else if(trendSpeed_<=-4) next=RivalReaction::FallingBehind;
        }
        // Never delay a safety/unknown transition. Smooth other text for 0.5s
        // to avoid message flapping from road junctions or variable framerate.
        if(next==current_) {proposed_=next;pendingSeconds_=0;}
        else if(next==RivalReaction::Racing || next==RivalReaction::Unavailable) {
            current_=proposed_=next;pendingSeconds_=0;out.changed=true;
        } else {
            if(next!=proposed_) {proposed_=next;pendingSeconds_=0;}
            pendingSeconds_+=in.elapsedSeconds;
            if(pendingSeconds_>=0.5f) {current_=next;pendingSeconds_=0;out.changed=true;}
        }
        out.state=current_;
        return out;
    }
};
} // namespace frr::domain
