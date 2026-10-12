#include "StreetSprint.h"
#include "RaceFormationReadiness.h"
#include <algorithm>
#include <cmath>

namespace frr::domain {
namespace {
bool finite(EncounterPoint p) {return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
float horizontal(EncounterPoint a,EncounterPoint b) {return std::hypot(a.x-b.x,a.z-b.z);}
}
bool StreetSprint::validCar(const EncounterCar& car) const {
    return car.valid && finite(car.position) && finite(car.forward) &&
        std::isfinite(car.speed) && car.speed>=0 && car.speed<=150;
}
StreetSprint::Progress StreetSprint::project(EncounterPoint p) const {
    const float dx=p.x-start_.x, dz=p.z-start_.z;
    return {dx*axis_.x+dz*axis_.z,dx*axis_.z-dz*axis_.x,p.y-start_.y};
}
bool StreetSprint::begin(const EncounterCar& player,const EncounterCar& rival) {
    reset();
    if(!validCar(player)||!validCar(rival)) return false;
    if(!std::isfinite(tuning_.lengthMeters)||tuning_.lengthMeters<60||
       tuning_.lengthMeters>500 || !std::isfinite(tuning_.corridorHalfWidthMeters) ||
       tuning_.corridorHalfWidthMeters<8 || tuning_.corridorHalfWidthMeters>30 ||
       !std::isfinite(tuning_.maximumSeconds) || tuning_.maximumSeconds<10 ||
       tuning_.maximumSeconds>180) return false;
    // Both vehicles must already be side-by-side and stationary in the
    // actual world BEFORE a native lease releases them.
    const auto formation=checkRaceFormation(player,rival,true);
    if(!formation.geometryAligned||!formation.bothStopped) return false;
    const float px=player.forward.x,pz=player.forward.z;
    const float rx=rival.forward.x,rz=rival.forward.z;
    const float norm=std::hypot(px+rx,pz+rz);
    if(norm<1.0f) return false;
    axis_={(px+rx)/norm,0,(pz+rz)/norm};
    start_={(player.position.x+rival.position.x)*0.5f,
            (player.position.y+rival.position.y)*0.5f,
            (player.position.z+rival.position.z)*0.5f};
    previousPlayer_=project(player.position);
    previousRival_=project(rival.position);
    lastPlayer_=player.position;
    lastRival_=rival.position;
    view_.distanceMeters=tuning_.lengthMeters;
    view_.finishPoint={start_.x+axis_.x*tuning_.lengthMeters,start_.y,
                       start_.z+axis_.z*tuning_.lengthMeters};
    active_=true;view_.running=true;view_.reason="race_active";
    return true;
}
StreetSprintView StreetSprint::finish(OutrunOutcome outcome,const char* reason) {
    active_=false;view_.running=false;view_.justFinished=true;
    view_.outcome=outcome;view_.reason=reason;
    return view_;
}
StreetSprintView StreetSprint::abort(const char* reason) {
    if(!active_) return view_;
    return finish(OutrunOutcome::Aborted,reason);
}
void StreetSprint::reset() {
    active_=false;view_={};
    view_.distanceMeters=tuning_.lengthMeters;
    start_={};axis_={};lastPlayer_={};lastRival_={};
    previousPlayer_={};previousRival_={};
}
StreetSprintView StreetSprint::tick(const StreetSprintFrame& input) {
    view_.justFinished=false;
    if(!active_) return view_;
    if(input.cancel) return abort("cancelled");
    if(!input.safeWorld||!validCar(input.player)||!validCar(input.rival))
        return abort("world_or_car_lost");
    const float dt=input.deltaSeconds;
    if(!std::isfinite(dt)||dt<=0||dt>0.5f)
        return abort("invalid_time");
    auto p=project(input.player.position),r=project(input.rival.position);
    // A driver may stop/reverse; continuous movement remains permitted.
    // Reject impossible position jumps rather than awarding a teleport win.
    if(horizontal(input.player.position,lastPlayer_)>
        std::max(input.player.speed,1.0f)*dt+7.0f ||
       horizontal(input.rival.position,lastRival_)>
        std::max(input.rival.speed,1.0f)*dt+7.0f)
        return abort("telemetry_jump");
    if(std::abs(p.vertical)>6 || std::abs(r.vertical)>6)
        return abort("different_elevation");
    // A straight course cannot be fairly scored when one driver takes an
    // unmapped curved street or shortcut outside the common corridor.
    if((p.distance>12 && std::abs(p.lateral)>tuning_.corridorHalfWidthMeters) ||
       (r.distance>12 && std::abs(r.lateral)>tuning_.corridorHalfWidthMeters))
        return abort("left_shared_corridor");
    view_.elapsed+=dt;
    view_.playerMeters=std::clamp(p.distance,0.0f,tuning_.lengthMeters);
    view_.rivalMeters=std::clamp(r.distance,0.0f,tuning_.lengthMeters);
    const bool pCross=previousPlayer_.distance<tuning_.lengthMeters &&
        p.distance>=tuning_.lengthMeters;
    const bool rCross=previousRival_.distance<tuning_.lengthMeters &&
        r.distance>=tuning_.lengthMeters;
    if(pCross||rCross) {
        if(pCross&&rCross) {
            const float pt=(tuning_.lengthMeters-previousPlayer_.distance)/
                std::max(p.distance-previousPlayer_.distance,0.001f);
            const float rt=(tuning_.lengthMeters-previousRival_.distance)/
                std::max(r.distance-previousRival_.distance,0.001f);
            if(std::abs(pt-rt)*dt<=0.03f) return finish(OutrunOutcome::Draw,"photo_finish");
            return finish(pt<rt?OutrunOutcome::PlayerWon:OutrunOutcome::RivalWon,"shared_finish_crossed");
        }
        return finish(pCross?OutrunOutcome::PlayerWon:OutrunOutcome::RivalWon,"shared_finish_crossed");
    }
    if(view_.elapsed>=tuning_.maximumSeconds) return abort("race_timeout");
    lastPlayer_=input.player.position;
    lastRival_=input.rival.position;
    previousPlayer_=p;
    previousRival_=r;
    return view_;
}
}
