#include "RivalGarage.h"

#include <algorithm>

namespace frr::domain {

const std::vector<RivalVehicleState>&
RivalGarage::vehicles() const {
    return vehicles_;
}

std::size_t RivalGarage::size() const {
    return vehicles_.size();
}

bool RivalGarage::empty() const {
    return vehicles_.empty();
}

const RivalVehicleState*
RivalGarage::activeVehicle() const {
    if (!activeId_) {
        return nullptr;
    }

    const auto it = std::find_if(
        vehicles_.begin(),
        vehicles_.end(),
        [&](const RivalVehicleState& v) {
            return v.instanceId == *activeId_;
        }
    );

    if (it == vehicles_.end()) {
        return nullptr;
    }

    return &*it;
}

std::optional<std::uint64_t>
RivalGarage::activeVehicleId() const {
    return activeId_;
}

bool RivalGarage::addVehicle(
    const RivalVehicleState& vehicle
) {
    if (vehicle.instanceId == 0 ||
        vehicle.vehicleKey.empty()) {
        return false;
    }

    const bool duplicate = std::any_of(
        vehicles_.begin(),
        vehicles_.end(),
        [&](const RivalVehicleState& existing) {
            return existing.instanceId == vehicle.instanceId;
        }
    );

    if (duplicate) {
        return false;
    }

    vehicles_.push_back(vehicle);

    if (!activeId_) {
        activeId_ = vehicle.instanceId;
    }

    return true;
}

bool RivalGarage::setActiveVehicle(
    std::uint64_t instanceId
) {
    const auto it = std::find_if(
        vehicles_.begin(),
        vehicles_.end(),
        [&](const RivalVehicleState& v) {
            return v.instanceId == instanceId;
        }
    );

    if (it == vehicles_.end()) {
        return false;
    }

    activeId_ = instanceId;
    return true;
}

bool RivalGarage::removeVehicle(
    std::uint64_t instanceId,
    bool protectLastVehicle
) {
    const auto it = std::find_if(
        vehicles_.begin(),
        vehicles_.end(),
        [&](const RivalVehicleState& v) {
            return v.instanceId == instanceId;
        }
    );

    if (it == vehicles_.end()) {
        return false;
    }

    if (protectLastVehicle && vehicles_.size() <= 1) {
        return false;
    }

    const bool removingActive =
        activeId_ &&
        *activeId_ == instanceId;

    vehicles_.erase(it);

    if (vehicles_.empty()) {
        activeId_.reset();
    } else if (removingActive) {
        activeId_ = vehicles_.front().instanceId;
    }

    return true;
}

bool RivalGarage::canWagerActiveVehicle() const {
    const auto* active = activeVehicle();

    if (!active ||
        !active->transferable) {
        return false;
    }

    std::size_t usable = 0;

    for (const auto& vehicle : vehicles_) {
        if (vehicle.transferable) {
            ++usable;
        }
    }

    return usable >= 2;
}

} // namespace frr::domain
