#pragma once
#include "OutrunRace.h"
#include <array>
#include <cstddef>

namespace frr::domain {
struct EncounterPoint { float x=0,y=0,z=0; };
struct EncounterCar {
    EncounterPoint position{},forward{};
    float speed=0;
    bool valid=false;
};
struct LiveEncounterInput {
    EncounterCar player{},rival{};
    float deltaSeconds=0;
    bool worldSafe=false,challengeSafe=false,acceptPressed=false,cancelPressed=false;
};
struct LiveEncounterView {
    bool available=false,running=false,routeMatched=false,justStarted=false,justFinished=false;
    const char* reason="none"; // Static reason identifiers, never borrowed input strings.
    OutrunOutcome outcome=OutrunOutcome::InProgress;
    OutrunLeader leader=OutrunLeader::Rival;
    float signedLeadMeters=0,holdProgress=0,remainingCooldown=0,elapsedSeconds=0;
};
// Mod-owned outrun scoring over the leading car's observed trail. This neither
// creates a vanilla event nor changes native AI targets, speed, cash or garage.
class LiveEncounter {
public:
    explicit LiveEncounter(OutrunTuning tuning={},float challengeDistance=60);
    LiveEncounterView tick(const LiveEncounterInput& input);
    LiveEncounterView interrupt(const char* reason="world_unsafe");
    bool running() const { return race_.running(); }
private:
    struct TrailPoint { EncounterPoint position{};float progress=0; };
    static constexpr std::size_t capacity=512;
    std::array<TrailPoint,capacity> trail_{};
    std::size_t head_=0,count_=0;
    float leaderProgress_=0,followerProgress_=0,unmatchedSeconds_=0,cooldown_=0,challengeDistance_=60;
    OutrunLeader leader_=OutrunLeader::Rival;
    EncounterCar previousPlayer_{},previousRival_{};
    OutrunRace race_;
    LiveEncounterView view_{};
    const TrailPoint& point(std::size_t index) const;
    void append(EncounterPoint point,float progress);
    void startTrail(const EncounterCar& follower,const EncounterCar& leader);
    LiveEncounterView finish(OutrunOutcome outcome);
};
}
