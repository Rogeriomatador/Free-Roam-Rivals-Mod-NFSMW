#include "PostRaceObservation.h"

#include <algorithm>
#include <cmath>

namespace frr::domain {
namespace {
bool valid(const ObservedRaceVehicle& v) {
    return v.vehicle && v.simable && v.model &&
        std::isfinite(v.x) && std::isfinite(v.y) && std::isfinite(v.z);
}
bool sameIdentity(const ObservedRaceVehicle& a, const ObservedRaceVehicle& b) {
    return a.vehicle == b.vehicle && a.simable == b.simable && a.model == b.model;
}
}

RegistrySnapshotStatus compareVehicleRegistrySnapshots(
    const VehicleRegistrySnapshot& before, const VehicleRegistrySnapshot& after) {
    for (const auto* s : {&before, &after}) {
        if (!s->complete || s->count != s->slots.size())
            return RegistrySnapshotStatus::Incomplete;
        if (s->count > 512 || (s->count && !s->storage))
            return RegistrySnapshotStatus::InvalidMembership;
        for (std::size_t i = 0; i < s->slots.size(); ++i) {
            if (!s->slots[i] || std::find(s->slots.begin(), s->slots.begin() + i,
                    s->slots[i]) != s->slots.begin() + i)
                return RegistrySnapshotStatus::InvalidMembership;
        }
    }
    if (before.storage != after.storage) return RegistrySnapshotStatus::StorageChanged;
    if (before.count != after.count) return RegistrySnapshotStatus::CountChanged;
    if (before.slots != after.slots) return RegistrySnapshotStatus::MembershipChanged;
    return RegistrySnapshotStatus::Stable;
}

const char* registrySnapshotStatusName(RegistrySnapshotStatus status) {
    switch (status) {
        case RegistrySnapshotStatus::Stable: return "Stable";
        case RegistrySnapshotStatus::Incomplete: return "Incomplete";
        case RegistrySnapshotStatus::InvalidMembership: return "InvalidMembership";
        case RegistrySnapshotStatus::StorageChanged: return "StorageChanged";
        case RegistrySnapshotStatus::CountChanged: return "CountChanged";
        case RegistrySnapshotStatus::MembershipChanged: return "MembershipChanged";
    }
    return "Unknown";
}

void PostRaceObserver::reset() {
    context_ = {};
    haveContext_ = haveClock_ = false;
    phase_ = RaceObservationPhase::Unavailable;
    lastMillis_ = lastRaceMillis_ = 0;
    entries_.clear();
    fadeSuspended_ = correlationInterrupted_ = false;
    fadeStartMillis_ = 0;
}

bool PostRaceObserver::suspendForRaceFade(std::uint64_t millis) {
    // A fade is not a lifetime bridge. Keep only a just-observed Racing cohort.
    if (!haveClock_ || phase_ != RaceObservationPhase::Racing || entries_.empty() ||
        millis <= lastMillis_ || millis - lastMillis_ > 2000 ||
        (fadeSuspended_ && millis - fadeStartMillis_ > 10000)) {
        reset();
        return false;
    }
    if (!fadeSuspended_) fadeStartMillis_ = millis;
    fadeSuspended_ = correlationInterrupted_ = true;
    lastMillis_ = millis;
    return true;
}

std::vector<PostRaceIdentityMatch> PostRaceObserver::observe(const RaceVehicleObservation& s) {
    std::vector<PostRaceIdentityMatch> out;
    bool invalid = !s.complete || s.phase == RaceObservationPhase::Unavailable ||
        !s.context.player || !s.context.roadNetwork || !s.context.raceStatus ||
        !s.context.profile || s.vehicles.size() > 512;
    for (std::size_t i = 0; !invalid && i < s.vehicles.size(); ++i) {
        invalid = !valid(s.vehicles[i]);
        for (std::size_t j = 0; !invalid && j < i; ++j)
            invalid = s.vehicles[i].vehicle == s.vehicles[j].vehicle ||
                s.vehicles[i].simable == s.vehicles[j].simable;
    }
    if (invalid) { reset(); return out; }
    if ((haveContext_ && !(s.context == context_)) ||
        (haveClock_ && (s.millis <= lastMillis_ || s.millis - lastMillis_ > 2000)) ||
        (fadeSuspended_ && (s.millis - fadeStartMillis_ > 10000 ||
            s.phase != RaceObservationPhase::Roaming))) {
        reset();
    }
    fadeSuspended_ = false;
    context_ = s.context;
    haveContext_ = haveClock_ = true;
    lastMillis_ = s.millis;

    if (s.phase == RaceObservationPhase::Racing) {
        correlationInterrupted_ = false;
        entries_.clear();
        for (const auto& v : s.vehicles)
            if (v.racer) entries_.push_back({v, {}, 0});
        lastRaceMillis_ = s.millis;
    } else if (phase_ == RaceObservationPhase::Racing || phase_ == RaceObservationPhase::Roaming) {
        // Evidence expires even if numeric addresses never change.
        if (s.millis - lastRaceMillis_ > 120000) entries_.clear();
        for (auto it = entries_.begin(); it != entries_.end();) {
            const auto match = std::find_if(s.vehicles.begin(), s.vehicles.end(),
                [&](const auto& v) { return sameIdentity(v, it->race); });
            if (match == s.vehicles.end()) { it = entries_.erase(it); continue; }
            PostRaceIdentityMatch result{};
            result.current = *match;
            result.correlationInterruptedByFade = correlationInterrupted_;
            result.millisSinceLastRaceSample = s.millis - lastRaceMillis_;
            result.aiIdentityChanged = match->ai != it->race.ai ||
                match->aiInterfaceVtable != it->race.aiInterfaceVtable;
            if (it->roamingSamples) {
                const float dx = match->x - it->lastRoaming.x;
                const float dy = match->y - it->lastRoaming.y;
                const float dz = match->z - it->lastRoaming.z;
                result.displacementSincePreviousRoamingSample = std::hypot(dx, dy, dz);
                result.displacementAvailable = std::isfinite(result.displacementSincePreviousRoamingSample);
            }
            result.roamingSamples = ++it->roamingSamples;
            it->lastRoaming = *match;
            out.push_back(result);
            ++it;
        }
    }
    phase_ = s.phase;
    return out;
}
} // namespace frr::domain
