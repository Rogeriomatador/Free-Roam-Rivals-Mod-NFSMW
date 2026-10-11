#pragma once
#include "LiveEncounter.h"
#include <cmath>
#include <algorithm>

namespace frr::domain {

// Only observation: unable to distinguish a road obstruction from an AI
// voluntarily stopping. Never used to authorize native teleports or AI writes.
struct RivalMotionState { bool stationaryLong=false;float secondsNearAnchor=0;bool changed=false; };
class RivalMotionMonitor {
    EncounterPoint anchor_{};
    float seconds_=0;
    bool have_=false,stationary_=false;
public:
    void reset(){have_=false;stationary_=false;seconds_=0;}
    RivalMotionState update(EncounterPoint at,float speed,float elapsedSeconds,bool reliable) {
        RivalMotionState out{};
        if(!reliable||!std::isfinite(speed)||speed<0||speed>150||
           !std::isfinite(elapsedSeconds)||elapsedSeconds<0||elapsedSeconds>1||
           !std::isfinite(at.x)||!std::isfinite(at.y)||!std::isfinite(at.z)){
            out.changed=stationary_;reset();return out;
        }
        if(!have_){anchor_=at;have_=true;return out;}
        const float movement=std::hypot(at.x-anchor_.x,at.z-anchor_.z);
        const float height=std::abs(at.y-anchor_.y);
        if(speed>3.0f||movement>5.0f||height>3.0f){
            out.changed=stationary_;
            reset();anchor_=at;have_=true;
            return out;
        }
        // "Almost stationary" can be temporary traffic. Require at least
        // twelve consecutive seconds, with position drifting under 5m.
        seconds_=std::min(300.0f,seconds_+elapsedSeconds);
        const bool flagged=seconds_>=12.0f;
        out.changed=(flagged!=stationary_);
        stationary_=flagged;
        out.stationaryLong=flagged;out.secondsNearAnchor=seconds_;
        return out;
    }
};
}
