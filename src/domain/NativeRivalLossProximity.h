#pragma once
#include <cmath>
#include <cstdint>

namespace frr::domain {
// Reports the *last verified observation*, not where the vehicle is now.
// Never authorizes native construction, deletion or a win/loss decision.
struct NativeRivalLossProximity {
    bool observationFresh=false;
    bool disappearedNearby=false;
    float lastVerifiedDistanceMeters=-1.0f;
};
inline NativeRivalLossProximity classifyNativeRivalLoss(
    std::uint64_t nowMs,std::uint64_t lastSeenMs,float lastDistanceMeters) {
    NativeRivalLossProximity evidence{};
    if(lastSeenMs==0||nowMs<lastSeenMs||nowMs-lastSeenMs>5000||
       !std::isfinite(lastDistanceMeters)||lastDistanceMeters<0)
        return evidence;
    evidence.observationFresh=true;
    evidence.lastVerifiedDistanceMeters=lastDistanceMeters;
    evidence.disappearedNearby=lastDistanceMeters<=250.0f;
    return evidence;
}
} // namespace frr::domain
