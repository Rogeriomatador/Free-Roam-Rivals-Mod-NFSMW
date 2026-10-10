#include "LiveEncounter.h"
#include <algorithm>
#include <cmath>
#include <limits>

namespace frr::domain {
namespace {
EncounterPoint subtract(EncounterPoint a,EncounterPoint b) {return {a.x-b.x,a.y-b.y,a.z-b.z};}
float dot(EncounterPoint a,EncounterPoint b) {return a.x*b.x+a.z*b.z;}
float distance(EncounterPoint a,EncounterPoint b) {const auto d=subtract(a,b);return std::sqrt(dot(d,d));}
bool finite(EncounterPoint p) {return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
bool valid(const EncounterCar& c) {
    const float length=dot(c.forward,c.forward);
    return c.valid&&finite(c.position)&&finite(c.forward)&&std::isfinite(c.speed)&&
        c.speed>=0&&c.speed<=150&&length>=0.8f&&length<=1.2f;
}
bool following(const EncounterCar& p,const EncounterCar& r,float maximum) {
    const auto offset=subtract(p.position,r.position);
    const float along=dot(offset,r.forward);
    const float lateral=std::abs(offset.x*r.forward.z-offset.z*r.forward.x);
    return valid(p)&&valid(r)&&p.speed>=3&&r.speed>=3&&dot(p.forward,r.forward)>=0.75f&&
        std::abs(offset.y)<=3&&along<=-1&&along>=-maximum&&lateral<=7&&
        distance(p.position,r.position)<=maximum&&std::abs(p.speed-r.speed)<=8.333334f;
}
bool continuous(const EncounterCar& old,const EncounterCar& next,float dt) {
    return distance(old.position,next.position)<=std::max(old.speed,next.speed)*dt+10&&
        std::abs(old.position.y-next.position.y)<=30*dt+4;
}
}
LiveEncounter::LiveEncounter(OutrunTuning tuning,float challengeDistance):
    challengeDistance_(std::isfinite(challengeDistance)?std::clamp(challengeDistance,10.0f,60.0f):60),race_(tuning) {}
const LiveEncounter::TrailPoint& LiveEncounter::point(std::size_t index) const {return trail_[(head_+index)%capacity];}
void LiveEncounter::append(EncounterPoint p,float progress) {
    if(count_==capacity) {head_=(head_+1)%capacity;--count_;}
    trail_[(head_+count_++)%capacity]={p,progress};
}
void LiveEncounter::startTrail(const EncounterCar& follower,const EncounterCar& leader) {
    head_=count_=0;followerProgress_=0;leaderProgress_=distance(follower.position,leader.position);
    append(follower.position,0);append(leader.position,leaderProgress_);unmatchedSeconds_=0;
}
LiveEncounterView LiveEncounter::finish(OutrunOutcome outcome) {
    view_.running=false;view_.justFinished=true;view_.outcome=outcome;
    cooldown_=15;view_.remainingCooldown=cooldown_;view_.available=false;
    return view_;
}
LiveEncounterView LiveEncounter::interrupt() {
    view_.justStarted=view_.justFinished=false;view_.available=false;
    if(race_.running()) {race_.abort();return finish(OutrunOutcome::Aborted);}
    return view_;
}
LiveEncounterView LiveEncounter::tick(const LiveEncounterInput& in) {
    view_.justStarted=view_.justFinished=false;
    if(!std::isfinite(in.deltaSeconds)||in.deltaSeconds<0||in.deltaSeconds>1||
        !in.worldSafe||!valid(in.player)||!valid(in.rival)) return interrupt();
    const float dt=in.deltaSeconds;
    if(dt==0) return view_;
    cooldown_=std::max(0.0f,cooldown_-dt);view_.remainingCooldown=cooldown_;
    view_.available=!race_.running()&&cooldown_==0&&in.challengeSafe&&following(in.player,in.rival,challengeDistance_);
    if(!race_.running()) {
        if(!view_.available||!in.acceptPressed) return view_;
        race_.begin();leader_=OutrunLeader::Rival;startTrail(in.player,in.rival);
        previousPlayer_=in.player;previousRival_=in.rival;
        view_.running=true;view_.justStarted=true;view_.outcome=OutrunOutcome::InProgress;
        view_.leader=leader_;view_.routeMatched=true;view_.signedLeadMeters=-leaderProgress_;view_.holdProgress=0;
        view_.elapsedSeconds=0;
        return view_;
    }
    if(in.cancelPressed||!continuous(previousPlayer_,in.player,dt)||!continuous(previousRival_,in.rival,dt)) {
        race_.abort();return finish(OutrunOutcome::Aborted);
    }
    const EncounterCar* leading=leader_==OutrunLeader::Player?&in.player:&in.rival;
    const EncounterCar* followingCar=leader_==OutrunLeader::Player?&in.rival:&in.player;
    const auto offset=subtract(followingCar->position,leading->position);
    const float lateral=std::abs(offset.x*leading->forward.z-offset.z*leading->forward.x);
    // Confirm a close overtake, then discard the previous leader's route.
    // Different roads, oncoming vehicles and overpasses cannot swap leaders.
    if(dot(offset,leading->forward)>5&&dot(leading->forward,followingCar->forward)>=0.75f&&
        lateral<=7&&std::abs(offset.y)<=3&&distance(leading->position,followingCar->position)<=30) {
        leader_=leader_==OutrunLeader::Player?OutrunLeader::Rival:OutrunLeader::Player;
        std::swap(leading,followingCar);startTrail(*followingCar,*leading);
    }
    const float advance=distance(point(count_-1).position,leading->position);
    if(advance>=2) {leaderProgress_+=advance;append(leading->position,leaderProgress_);}
    const float possibleAdvance=distance(previousPlayer_.position,in.player.position)+
        distance(previousRival_.position,in.rival.position)+10;
    float bestDistance=std::numeric_limits<float>::max(),bestProgress=followerProgress_;
    for(std::size_t i=1;i<count_;++i) {
        const auto& a=point(i-1);const auto& b=point(i);
        const auto delta=subtract(b.position,a.position);
        const float length=dot(delta,delta);
        if(length<0.01f) continue;
        const float t=std::clamp(dot(subtract(followingCar->position,a.position),delta)/length,0.0f,1.0f);
        const float progress=a.progress+(b.progress-a.progress)*t;
        if(std::abs(progress-followerProgress_)>possibleAdvance) continue;
        const EncounterPoint projection{a.position.x+t*delta.x,a.position.y+t*delta.y,a.position.z+t*delta.z};
        if(std::abs(projection.y-followingCar->position.y)>4) continue;
        const float error=distance(projection,followingCar->position);
        if(error<bestDistance) {bestDistance=error;bestProgress=progress;}
    }
    view_.routeMatched=bestDistance<=12;
    if(view_.routeMatched) {followerProgress_=bestProgress;unmatchedSeconds_=0;}
    else unmatchedSeconds_+=dt;
    previousPlayer_=in.player;previousRival_=in.rival;
    if(unmatchedSeconds_>=3) {race_.abort();return finish(OutrunOutcome::Aborted);}
    view_.leader=leader_;
    view_.signedLeadMeters=(leader_==OutrunLeader::Player?1.0f:-1.0f)*std::max(0.0f,leaderProgress_-followerProgress_);
    // An unmatched route resets the hold timer, rather than awarding a win for
    // lateral separation. Uncertainty for three seconds cancels the battle.
    const auto result=race_.tick({dt,view_.routeMatched?view_.signedLeadMeters:0,true,true,false,false});
    view_.holdProgress=race_.holdProgress01();view_.elapsedSeconds=race_.elapsedSeconds();
    if(result!=OutrunOutcome::InProgress) {
        // A timeout while the cars cannot be matched to one observed route
        // must not manufacture a draw or award a result from uncertain data.
        if(!view_.routeMatched) {race_.abort();return finish(OutrunOutcome::Aborted);}
        return finish(result);
    }
    return view_;
}
}
