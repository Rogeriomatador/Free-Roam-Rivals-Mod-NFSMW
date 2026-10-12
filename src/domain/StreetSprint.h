#pragma once
#include "LiveEncounter.h"
#include "OutrunRace.h"

namespace frr::domain {

// A local, physically scored straight-road duel: two independent in-world
// vehicle positions cross ONE shared finish plane. Not the game's stock
// race event, and not a path-finding promise on curved Rockport streets.
struct StreetSprintTuning {
    float lengthMeters=120.0f;
    float corridorHalfWidthMeters=16.0f;
    float maximumSeconds=45.0f;
};
struct StreetSprintFrame {
    EncounterCar player{}, rival{};
    float deltaSeconds=0.0f;
    bool safeWorld=false, cancel=false;
};
struct StreetSprintView {
    bool running=false, justFinished=false;
    OutrunOutcome outcome=OutrunOutcome::InProgress;
    const char* reason="not_started";
    float elapsed=0, playerMeters=0, rivalMeters=0;
    float distanceMeters=120;
    EncounterPoint finishPoint{};
};
class StreetSprint {
public:
    explicit StreetSprint(StreetSprintTuning tuning={}):tuning_(tuning){}
    bool begin(const EncounterCar& player,const EncounterCar& rival);
    StreetSprintView tick(const StreetSprintFrame& input);
    StreetSprintView abort(const char* reason="interrupted");
    void reset();
    bool running() const {return active_;}
    const StreetSprintView& view() const {return view_;}
private:
    struct Progress {float distance=0,lateral=0,vertical=0;};
    Progress project(EncounterPoint position) const;
    bool validCar(const EncounterCar& car) const;
    StreetSprintView finish(OutrunOutcome outcome,const char* reason);
    StreetSprintTuning tuning_{};
    StreetSprintView view_{};
    bool active_=false;
    EncounterPoint start_{}, axis_{}, lastPlayer_{}, lastRival_{};
    Progress previousPlayer_{}, previousRival_{};
};
}
