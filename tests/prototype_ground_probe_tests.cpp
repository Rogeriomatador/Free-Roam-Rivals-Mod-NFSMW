#include "domain/PrototypeGroundProbe.h"
#include <cstdio>

int main() {
    using namespace frr::domain;
    const SpatialVector3 point{10, 40, 20};
    const auto ray = prototypeGroundSegment(point);
    if (ray[0].y != 42 || ray[1].y != 36 || ray[0].x != 10 || ray[1].z != 20) return 1;
    WorldCollisionSample sample{};
    sample.callAvailable = sample.callCompleted = sample.hit = true;
    sample.hitType = 1; sample.hitPoint = {10, 40, 20};
    // Analytic flat-face regression: the native normal faces the origin.
    sample.normal = {0, ray[0].y > sample.hitPoint.y ? 1.0f : -1.0f, 0};
    if (!prototypeGroundAcceptable(interpretGroundCollision(point, sample))) return 2;
    // Old fallback started below the road and produced the rejected sign.
    sample.normal = {0, -1, 0};
    if (prototypeGroundAcceptable(interpretGroundCollision(point, sample))) return 3;
    sample.normal = {0, 1, 0}; sample.hitPoint.y = 38;
    if (prototypeGroundAcceptable(interpretGroundCollision(point, sample))) return 4;
    sample.hitPoint.y = 40; sample.normal = {0, 0.8f, 0.6f};
    if (prototypeGroundAcceptable(interpretGroundCollision(point, sample))) return 5;
    sample.normal = {0, 1, 0}; sample.hitType = 2;
    if (prototypeGroundAcceptable(interpretGroundCollision(point, sample))) return 6;
    sample.hitType = 1; sample.hit = false;
    if (prototypeGroundAcceptable(interpretGroundCollision(point, sample))) return 7;
    std::puts("Prototype ground ray regression passed: downward/upward normal, height, slope, barrier and missing ground.");
    return 0;
}
