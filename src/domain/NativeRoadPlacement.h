#pragma once
#include "VehicleSpatialEvidence.h"
#include <algorithm>
#include <cmath>

namespace frr::domain {

// Runtime ResetToRoad may project a nav seed onto a physical lane/rigid-body
// center. This is only a bounded preliminary geometry gate. A positive result
// MUST still pass fresh full-footprint ground + fleet clearance BEFORE Activate.
// All coordinates are MW05 canonical world units, NOT assumed metres.
struct NativeRoadPlacement {
    bool valid=false;
    bool relocated=false;
    float horizontalDelta=0,verticalDelta=0,forwardDelta=0,lateralDelta=0;
    const char* reason="invalid_sample";
};
inline NativeRoadPlacement inspectNativeRoadPlacement(
    SpatialVector3 navPosition,SpatialVector3 navForward,
    const VehicleOrientedBox& actual) {
    NativeRoadPlacement result{};
    if(!actual.valid) return result;
    for(float n:{navPosition.x,navPosition.y,navPosition.z,navForward.x,navForward.y,
        navForward.z,actual.center.x,actual.center.y,actual.center.z,
        actual.forward.x,actual.forward.y,actual.forward.z,
        actual.halfExtents.x,actual.halfExtents.y,actual.halfExtents.z})
        if(!std::isfinite(n)) return result;
    const float n=std::hypot(navForward.x,navForward.z);
    const float a=std::hypot(actual.forward.x,actual.forward.z);
    if(n<0.5f||a<0.5f || actual.halfExtents.x<=0||
       actual.halfExtents.y<=0||actual.halfExtents.z<=0||
       actual.halfExtents.x>3||actual.halfExtents.y>2||actual.halfExtents.z>8) {
        result.reason="dimensions_or_direction_invalid";
        return result;
    }
    const float nx=navForward.x/n,nz=navForward.z/n;
    const float dx=actual.center.x-navPosition.x,dz=actual.center.z-navPosition.z;
    result.horizontalDelta=std::hypot(dx,dz);
    result.verticalDelta=std::abs(actual.center.y-navPosition.y);
    result.forwardDelta=std::abs(dx*nx+dz*nz);
    result.lateralDelta=std::abs(dx*nz-dz*nx);
    result.relocated=result.horizontalDelta>1.0f;
    const float longitudinalBound=std::min(9.0f,std::max(4.0f,actual.halfExtents.z*2.0f+2.0f));
    const float lateralBound=std::min(6.0f,std::max(3.0f,actual.halfExtents.x*2.0f+2.0f));
    if(result.horizontalDelta>9.0f || result.forwardDelta>longitudinalBound ||
       result.lateralDelta>lateralBound || result.verticalDelta>2.5f) {
        result.reason="relocation_exceeds_vehicle_footprint";
        return result;
    }
    // A large offset with a vehicle pointing the other way is not a
    // validated projection onto the same carriageway.
    if(result.relocated && (nx*actual.forward.x+nz*actual.forward.z)/a<0.4f) {
        result.reason="post_reset_heading_inconsistent";
        return result;
    }
    result.valid=true;result.reason=result.relocated ?
        "bounded_projection_requires_full_revalidation":"within_exact_anchor_gate";
    return result;
}
}
