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
    VehicleRegistrySnapshot before{100, 2, true, {10, 11}};
    auto after = before;
    require(compareVehicleRegistrySnapshots(before, after) == RegistrySnapshotStatus::Stable,
        "unchanged complete membership can be accepted");
    after.slots[1] = 12;
    require(compareVehicleRegistrySnapshots(before, after) == RegistrySnapshotStatus::MembershipChanged,
        "a same-count replacement is detected");
    after = before; after.slots = {11, 10};
    require(compareVehicleRegistrySnapshots(before, after) == RegistrySnapshotStatus::MembershipChanged,
        "same-size registry reordering is not silently accepted");
    after = before; after.storage++;
    require(compareVehicleRegistrySnapshots(before, after) == RegistrySnapshotStatus::StorageChanged,
        "relocated registry storage invalidates the sample");
    after = before; after.count = 1; after.slots.pop_back();
    require(compareVehicleRegistrySnapshots(before, after) == RegistrySnapshotStatus::CountChanged,
        "changed registry count invalidates the sample");
    after = before; after.complete = false;
    require(compareVehicleRegistrySnapshots(before, after) == RegistrySnapshotStatus::Incomplete,
        "failed second reads are not disappearance evidence");
    after = before; after.slots.pop_back();
    require(compareVehicleRegistrySnapshots(before, after) == RegistrySnapshotStatus::Incomplete,
        "a partial traversal cannot claim complete membership");
    for (int fault = 0; fault < 4; ++fault) {
        after = before;
        if (fault == 0) after.storage = 0;
        if (fault == 1) after.slots[1] = 0;
        if (fault == 2) after.slots[1] = after.slots[0];
        if (fault == 3) { after.count = 513; after.slots.resize(513, 12); }
        require(compareVehicleRegistrySnapshots(before, after) == RegistrySnapshotStatus::InvalidMembership,
            "null, duplicate, oversized or storage-less registries fail closed");
    }
    before = {0, 0, true, {}}; after = before;
    require(compareVehicleRegistrySnapshots(before, after) == RegistrySnapshotStatus::Stable,
        "a complete empty list remains distinct from a failed read");
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
    // Replay the failure found in the actual v30 capture: Racing -> fade -> Roaming.
    observer.reset(); s = racing(); observer.observe(s);
    require(observer.suspendForRaceFade(1500), "race fade preserves numeric evidence");
    require(observer.suspendForRaceFade(2000), "subsequent fade sample retains bounded evidence");
    s.phase = RaceObservationPhase::Roaming; s.millis = 2500;
    matches = observer.observe(s);
    require(matches.size() == 1 && matches[0].correlationInterruptedByFade &&
        !matches[0].displacementAvailable, "resume is explicitly interrupted, never race-to-roam motion");
    s.millis += 500; s.vehicles[0].x += 3;
    matches = observer.observe(s);
    require(matches.size() == 1 && matches[0].correlationInterruptedByFade &&
        matches[0].displacementAvailable, "only resumed roaming samples measure displacement");
    require(!observer.suspendForRaceFade(3500), "fade after roaming revokes instead of bridging");
    for (int fault = 0; fault < 5; ++fault) {
        observer.reset(); s = racing(); observer.observe(s);
        require(observer.suspendForRaceFade(1500), "setup race-end fade");
        s.phase = RaceObservationPhase::Roaming; s.millis = 2000;
        if (fault == 0) s.context.profile++;
        if (fault == 1) s.context.player++;
        if (fault == 2) s.complete = false;
        if (fault == 3) s.vehicles[0].simable++;
        if (fault == 4) s.phase = RaceObservationPhase::Unavailable;
        require(observer.observe(s).empty(), "fade retention does not bypass invalid/context checks");
    }
    observer.reset(); s = racing(); observer.observe(s);
    for (std::uint64_t t = 1500; t <= 11500; t += 500)
        require(observer.suspendForRaceFade(t), "fade within total deadline");
    require(!observer.suspendForRaceFade(12000) && observer.capturedRaceCount() == 0,
        "repeated fades cannot extend the ten-second deadline");
    s.phase = RaceObservationPhase::Roaming; s.millis = 12500;
    require(observer.observe(s).empty(), "expired archive cannot resurrect addresses");
    observer.reset(); s = racing(); observer.observe(s);
    require(!observer.suspendForRaceFade(4001), "unobserved clock gap still revokes");
    std::cout << "Post-race observation tests passed\n";
}
