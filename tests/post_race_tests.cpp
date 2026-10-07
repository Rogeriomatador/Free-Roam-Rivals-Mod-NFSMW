#include "domain/PostRaceObservation.h"
#include <cstdlib>
#include <iostream>
#include <limits>
using namespace frr::domain;
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
RaceVehicleObservation racing() {
    RaceVehicleObservation s{};
    s.context = {1, 2, 3, 4}; s.phase = RaceObservationPhase::Racing;
    s.millis = 1000; s.complete = true;
    s.vehicles = {{10, 20, 30, 4, true, 1, 2, 3, 40, 50}};
    return s;
}
int main() {
    PostRaceObserver observer;
    auto s = racing();
    require(observer.observe(s).empty(), "a racing snapshot never claims a post-race match");
    s.phase = RaceObservationPhase::Roaming; s.millis += 500;
    auto matches = observer.observe(s);
    require(matches.size() == 1 && matches[0].roamingSamples == 1 &&
        !matches[0].displacementAvailable, "first match cannot claim motion from a race teleport");
    s.millis += 500; s.vehicles[0].x += 3; s.vehicles[0].y += 4;
    matches = observer.observe(s);
    require(matches.size() == 1 && matches[0].displacementAvailable &&
        matches[0].displacementSincePreviousRoamingSample == 5,
        "movement is measured between consecutive roaming samples");
    s.millis += 500; s.vehicles[0].ai++;
    require(observer.observe(s)[0].aiIdentityChanged, "AI replacement is reported instead of hidden");
    s.millis += 500; s.vehicles.clear();
    require(observer.observe(s).empty(), "missing vehicle revokes its correlation");
    s.millis += 500; s.vehicles = racing().vehicles;
    require(observer.observe(s).empty(), "reused addresses cannot resurrect an absent vehicle");
    for (int fault = 0; fault < 12; ++fault) {
        observer.reset(); s = racing(); observer.observe(s);
        s.phase = RaceObservationPhase::Roaming; s.millis += 500;
        if (fault == 0) s.context.player++;
        if (fault == 1) s.context.roadNetwork++;
        if (fault == 2) s.context.raceStatus++;
        if (fault == 3) s.context.profile++;
        if (fault == 4) s.complete = false;
        if (fault == 5) s.phase = RaceObservationPhase::Unavailable;
        if (fault == 6) s.millis += 3000;
        if (fault == 7) s.millis = 900;
        if (fault == 8) s.vehicles[0].model++;
        if (fault == 9) s.vehicles[0].simable++;
        if (fault == 10) s.vehicles[0].x = std::numeric_limits<float>::quiet_NaN();
        if (fault == 11) s.vehicles.push_back(s.vehicles[0]);
        require(observer.observe(s).empty(), "invalid or ambiguous observation cannot correlate");
    }
    observer.reset(); s = racing(); s.vehicles[0].racer = false; observer.observe(s);
    s.phase = RaceObservationPhase::Roaming; s.millis += 500;
    require(observer.observe(s).empty(), "traffic seen during a race is not labelled a former rival");
    observer.reset(); s = racing(); observer.observe(s); s.phase = RaceObservationPhase::Roaming;
    for (int i = 0; i < 240; ++i) { s.millis += 500; observer.observe(s); }
    s.millis += 500;
    require(observer.observe(s).empty(), "correlation expires after two minutes");
    std::cout << "Post-race observation tests passed\n";
}
