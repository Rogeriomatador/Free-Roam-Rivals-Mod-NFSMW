#pragma once

#include <cstdint>
#include <vector>

namespace frr::domain {

enum class RaceObservationPhase { Unavailable, Racing, Roaming };

// Values copied from the live registry. Matching two reads detects observed
// storage/membership changes; it is not an atomic snapshot or a lifetime proof.
struct VehicleRegistrySnapshot {
    std::uintptr_t storage = 0;
    std::uint32_t count = 0;
    bool complete = false;
    std::vector<std::uintptr_t> slots{};
};
enum class RegistrySnapshotStatus {
    Stable, Incomplete, InvalidMembership, StorageChanged, CountChanged, MembershipChanged
};
RegistrySnapshotStatus compareVehicleRegistrySnapshots(
    const VehicleRegistrySnapshot& before, const VehicleRegistrySnapshot& after);
const char* registrySnapshotStatusName(RegistrySnapshotStatus status);

struct RaceObservationContext {
    std::uintptr_t player = 0;
    std::uintptr_t roadNetwork = 0;
    std::uintptr_t raceStatus = 0;
    std::uint64_t profile = 0;
    bool operator==(const RaceObservationContext&) const = default;
};

// Numeric identities only. This layer never dereferences or owns game objects.
struct ObservedRaceVehicle {
    std::uintptr_t vehicle = 0;
    std::uintptr_t simable = 0;
    std::uint32_t model = 0;
    std::uint32_t driverClass = 0;
    bool racer = false;
    float x = 0, y = 0, z = 0;
    std::uintptr_t ai = 0;
    std::uintptr_t aiInterfaceVtable = 0;
};

struct RaceVehicleObservation {
    RaceObservationContext context{};
    RaceObservationPhase phase = RaceObservationPhase::Unavailable;
    std::uint64_t millis = 0;
    bool complete = false;
    std::vector<ObservedRaceVehicle> vehicles{};
};

struct PostRaceIdentityMatch {
    ObservedRaceVehicle current{};
    std::uint64_t millisSinceLastRaceSample = 0;
    std::uint32_t roamingSamples = 0;
    float displacementSincePreviousRoamingSample = 0;
    bool displacementAvailable = false;
    bool aiIdentityChanged = false;
    bool correlationInterruptedByFade = false;
};

class PostRaceObserver {
public:
    // Correlation across consecutive samples, NOT proof of uninterrupted object
    // lifetime. Short race-end fades retain explicitly interrupted evidence.
    // Absence, incomplete reads, loading, context changes, and other gaps
    // revoke matches. No mutation/spawn-readiness gate consumes these results.
    std::vector<PostRaceIdentityMatch> observe(const RaceVehicleObservation& sample);
    void reset();
    // Preserve plain numeric evidence, never game read authorization.
    bool suspendForRaceFade(std::uint64_t millis);
    std::size_t capturedRaceCount() const { return entries_.size(); }
private:
    struct Entry {
        ObservedRaceVehicle race{};
        ObservedRaceVehicle lastRoaming{};
        std::uint32_t roamingSamples = 0;
    };
    RaceObservationContext context_{};
    bool haveContext_ = false;
    bool haveClock_ = false;
    RaceObservationPhase phase_ = RaceObservationPhase::Unavailable;
    std::uint64_t lastMillis_ = 0;
    std::uint64_t lastRaceMillis_ = 0;
    std::vector<Entry> entries_{};
    bool fadeSuspended_ = false;
    bool correlationInterrupted_ = false;
    std::uint64_t fadeStartMillis_ = 0;
};
} // namespace frr::domain
