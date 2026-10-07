#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace frr::domain {

struct RivalVehicleState {
    std::uint64_t instanceId = 0;
    std::string vehicleKey;

    // Seeds/snapshots are mod-owned identity metadata. They do not imply
    // ownership in the game's career garage.
    std::uint64_t visualSeed = 0;
    std::uint64_t performanceSeed = 0;

    std::int64_t streetValue = 0;

    bool wonFromPlayer = false;
    bool legendary = false;
    bool transferable = true;
};

class RivalGarage {
public:
    const std::vector<RivalVehicleState>& vehicles() const;

    std::size_t size() const;
    bool empty() const;

    const RivalVehicleState* activeVehicle() const;
    std::optional<std::uint64_t> activeVehicleId() const;

    bool addVehicle(const RivalVehicleState& vehicle);
    bool setActiveVehicle(std::uint64_t instanceId);

    // Default removal protects the final usable vehicle, matching the
    // player's future pink-slip rule.
    bool removeVehicle(
        std::uint64_t instanceId,
        bool protectLastVehicle = true
    );

    bool canWagerActiveVehicle() const;

private:
    std::vector<RivalVehicleState> vehicles_;
    std::optional<std::uint64_t> activeId_;
};

} // namespace frr::domain
