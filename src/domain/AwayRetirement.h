#pragma once
#include <cmath>

namespace frr::domain {

// Near-player prototype only. Probe BEFORE streaming may unload the owned car.
// A probe is read-only; the caller still verifies the camera/world occlusion,
// the native registries, matching world ownership and actual Kill ABI.
struct AwayRetirementInput {
    bool nearPlayerPrototype=false;
    bool nativeCarActive=false, loaded=false, validBox=false;
    bool worldAndPursuitClear=false, contestRunning=false;
    unsigned stableSamples=0;
    float verifiedDistanceMeters=0;
};
inline bool shouldProbeSafeAwayRetirement(const AwayRetirementInput& in) {
    return in.nearPlayerPrototype && in.nativeCarActive && in.loaded &&
        in.validBox && in.worldAndPursuitClear && !in.contestRunning &&
        in.stableSamples>=6 && std::isfinite(in.verifiedDistanceMeters) &&
        in.verifiedDistanceMeters>=350.0f &&
        in.verifiedDistanceMeters<=850.0f;
}
} // namespace frr::domain
