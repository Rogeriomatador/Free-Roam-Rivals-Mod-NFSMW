#include "domain/RuntimeEvidence.h"
#include "domain/SelectedRivalEvidence.h"
#include "domain/RivalPopulation.h"
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
}
int main() {
    using namespace frr::domain;
    RuntimeEvidenceStamp captured{true, 1, 10, 20, 30, 40, 50, 1000};
    require(runtimeEvidenceUsable(captured, captured, 1500), "fresh lease accepts inclusive age limit");
    require(!runtimeEvidenceUsable(captured, captured, 1501), "expired request rejected");
    require(!runtimeEvidenceUsable(captured, captured, 999), "clock regression rejected");
    for (int i = 0; i < 8; ++i) {
        auto changed = captured;
        switch (i) {
            case 0: changed.safeFreeRoam = false; break;
            case 1: ++changed.generation; break;
            case 2: ++changed.playerIVehicle; break;
            case 3: ++changed.playerPVehicle; break;
            case 4: ++changed.roadNetwork; break;
            case 5: ++changed.raceStatus; break;
            case 6: ++changed.profileKey; break;
            case 7: changed.generation = 0; break;
        }
        require(!runtimeEvidenceUsable(captured, changed, 1100), "world transition cannot reuse lease");
    }
    require(!runtimeEvidenceUsable({}, captured, 1100), "empty request rejected");

    ProceduralRivalRequest request{};
    request.seed = 741;
    const auto rival = generateProceduralRival(request);
    const auto repeat = generateProceduralRival(request);
    require(rival && repeat && rival->rivalId == repeat->rivalId &&
            rival->vehicleKey == repeat->vehicleKey, "pending rival selection deterministic");
    VehicleOrientedBox live{};
    live.valid = true; live.identity = 1; live.vehicleKey = 100;
    live.right = {1, 0, 0}; live.up = {0, 1, 0}; live.forward = {0, 0, 1};
    live.halfExtents = {1, 1, 2};
    VehicleFootprintLearner learner;
    for (int i = 0; i < 4; ++i) learner.observe(live);
    RoadCandidateObservation road{};
    road.positionAvailable = road.position.finite = road.forward.finite = true;
    road.roadGeometryAvailable = road.exactRoadGeometry = road.roadValid = true;
    road.forwardProjectionWorldUnits = road.distanceWorldUnits = 10;
    road.position = {10, 0, 0, true}; road.forward = {0, 0, 1, true};
    auto report = evaluateSelectedRivalOverlap({rival->rivalId, 200}, road, learner, {live}, true);
    require(!report.footprintVerified && !report.evidence.overlapVerified && report.evidence.overlapsLiveVehicle,
            "learned alternative cannot replace unlearned selected model");
    report = evaluateSelectedRivalOverlap({rival->rivalId, 100}, road, learner, {live}, true);
    require(report.footprintVerified && report.evidence.overlapVerified && !report.evidence.overlapsLiveVehicle,
            "selected learned model promotes full-fleet clear overlap");
    require(!report.evidence.streamingVerified && !report.evidence.visibilityVerified && !report.evidence.groundVerified,
            "overlap cannot imply other evidence");
    road.position.x = 0;
    report = evaluateSelectedRivalOverlap({rival->rivalId, 100}, road, learner, {live}, true);
    require(report.evidence.overlapVerified && report.evidence.overlapsLiveVehicle,
            "occupied selected footprint rejected");
    road.position.x = 10;
    report = evaluateSelectedRivalOverlap({rival->rivalId, 100}, road, learner, {live}, false);
    require(!report.evidence.overlapVerified && report.evidence.overlapsLiveVehicle, "incomplete fleet fail-closed");
    road.exactRoadGeometry = false;
    require(!evaluateSelectedRivalOverlap({rival->rivalId, 100}, road, learner, {live}, true).footprintVerified,
            "lookahead without exact association cannot promote");
    road.exactRoadGeometry = true;
    require(!evaluateSelectedRivalOverlap({0, 100}, road, learner, {live}, true).footprintVerified,
            "missing rival identity rejected");
    // Within-spread observations must use the maximum, never the smaller mean.
    live.halfExtents.x = 1.02f;
    learner.observe(live);
    report = evaluateSelectedRivalOverlap({rival->rivalId, 100}, road, learner, {live}, true);
    require(report.footprintVerified && report.footprint.halfExtents.x >= 1.02f, "envelope covers largest observation");
    live.halfExtents.x = 5;
    learner.observe(live);
    require(!evaluateSelectedRivalOverlap({rival->rivalId, 100}, road, learner, {live}, true).footprintVerified,
            "later unstable dimensions revoke learned footprint");
    std::cout << "Free Roam Rivals evidence tests passed.\n";
}
