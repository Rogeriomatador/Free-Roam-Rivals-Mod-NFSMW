#include "domain/MotionScaleObserver.h"
#include "domain/RuntimeEvidence.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
frr::domain::MotionScaleFrame frame() {
    frr::domain::MotionScaleFrame f{};
    f.valid = f.safeFreeRoam = f.grounded = true;
    f.engineSpeed = f.speedometer = f.absoluteSpeed = 10.0f;
    f.localVelocityMagnitude = f.linearVelocityMagnitude = 10.0f;
    return f;
}
}
int main() {
    using namespace frr::domain;
    MotionScaleTuning t{};
    t.minimumStableSamples = 4;
    t.maximumWindowSamples = 4;
    MotionScaleObserver observer(t);
    auto f = frame();
    observer.push(f, 1.0f);
    MotionScaleSnapshot s{};
    for (int i = 0; i < 20; ++i) { f.x += 10; s = observer.push(f, 1); }
    require(s.acceptedSamples == 20 && s.windowSamples == 4 && s.stable,
            "window is bounded while cohort totals remain available");
    for (int i = 0; i < 4; ++i) { f.x += 5; s = observer.push(f, 1); }
    require(s.windowSamples == 4 && std::abs(s.meanWorldUnitsPerSpeedUnitSecond - .5f) < .0001f,
            "new timing regime replaces older observations rather than being diluted");
    f.grounded = false;
    s = observer.push(f, 1);
    require(!s.stable && !s.lastPairAccepted && s.windowSamples == 0 && s.acceptedSamples == 24,
            "rejected pair revokes stability immediately");
    f.grounded = true;
    f.x += 10;
    require(!observer.push(f, 1).lastPairAccepted, "airborne previous endpoint remains rejected");
    for (int i = 0; i < 4; ++i) { f.x += 10; s = observer.push(f, 1); }
    require(s.stable, "a fresh continuous window can recover");
    for (int channel = 0; channel < 8; ++channel) {
        MotionScaleObserver bad(t);
        auto first = frame(), next = first;
        bad.push(first, 1);
        next.x = 10;
        const auto nan = std::numeric_limits<float>::quiet_NaN();
        switch (channel) {
            case 0: next.x = nan; break;
            case 1: next.y = nan; break;
            case 2: next.z = nan; break;
            case 3: next.engineSpeed = nan; break;
            case 4: next.speedometer = nan; break;
            case 5: next.absoluteSpeed = nan; break;
            case 6: next.localVelocityMagnitude = nan; break;
            case 7: next.linearVelocityMagnitude = nan; break;
        }
        s = bad.push(next, 1);
        require(s.acceptedSamples == 0 && s.rejectedSamples == 1 && !s.lastPairAccepted,
                "every nonfinite channel is rejected without a false zero cross-check");
    }
    for (int reason = 0; reason < 6; ++reason) {
        MotionScaleObserver bad(t);
        auto first = frame(), next = first;
        bad.push(first, 1);
        next.x = 10;
        float dt = 1;
        switch (reason) {
            case 0: next.engineSpeed = -10; break;
            case 1: next.localVelocityMagnitude = 0; break;
            case 2: next.linearVelocityMagnitude = -1; break;
            case 3: next.safeFreeRoam = false; break;
            case 4: dt = .01f; break;
            case 5: dt = 3; break;
        }
        require(!bad.push(next, dt).lastPairAccepted, "invalid pair cannot contribute to calibration");
    }
    RuntimeEvidenceStamp context{true, 1, 10, 20, 30, 40, 50, 100};
    auto next = context;
    next.capturedAtMillis = 10000;
    require(sameRuntimeEvidenceContext(context, next), "capture identity does not depend on lease age");
    require(!runtimeEvidenceUsable(context, next, 10000), "identity match still cannot bypass lease expiry");
    ++next.playerPVehicle;
    require(!sameRuntimeEvidenceContext(context, next), "player replacement splits capture cohort");
    next = context; ++next.profileKey;
    require(!sameRuntimeEvidenceContext(context, next), "profile replacement splits capture cohort");
    observer.reset();
    s = observer.snapshot();
    require(!s.stable && s.windowSamples == 0 && s.acceptedSamples == 0 && s.rejectedSamples == 0,
            "reset clears all cohort data");
    std::cout << "Motion tests passed\n";
}
