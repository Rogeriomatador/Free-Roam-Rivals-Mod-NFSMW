#include "domain/VehicleSpatialEvidence.h"
#include <cstdlib>
#include <iostream>
#include <limits>
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
int main() {
    using namespace frr::domain;
    const SpatialVector3 center{5,10,15}, right{1,0,0}, up{0,1,0}, forward{0,0,1}, extent{1,1,2};
    const auto box=makeVehicleOrientedBox(42,17,center,right,up,forward,extent);
    require(box.valid && box.identity==42 && box.vehicleKey==17, "fresh valid runtime geometry is accepted with exact identity/model");
    auto report=evaluatePointAgainstFleet(center,{box},true);
    require(report.verified && report.insideAnyVehicle && report.checkedVehicles==1, "runtime-created box participates in real fleet occupancy");
    require(!evaluatePointAgainstFleet(center,{box},false).verified, "geometry cannot promote incomplete fleet");
    require(!makeVehicleOrientedBox(42,17,center,{},up,forward,extent).valid, "zero axis rejected");
    require(!makeVehicleOrientedBox(42,17,center,right,right,forward,extent).valid, "non-orthogonal basis rejected");
    require(!makeVehicleOrientedBox(42,17,center,right,up,forward,{1,0,2}).valid, "nonpositive extent rejected");
    require(!makeVehicleOrientedBox(42,17,{std::numeric_limits<float>::quiet_NaN(),0,0},right,up,forward,extent).valid, "nonfinite center rejected");
    auto invalid=box; invalid.valid=false;
    require(!validVehicleOrientedBox(invalid), "existing invalid observations cannot bypass validity marker");
    std::cout << "Vehicle spatial construction tests passed\n";
}
