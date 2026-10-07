#include "RivalRuntimeHandle.h"

namespace frr::domain {

void RivalRuntimeHandle::bind(
    std::uint64_t rivalId,
    std::uint64_t worldGeneration,
    RivalRuntimePointers pointers,
    bool ownsSpawnedVehicle
) {
    rivalId_ = rivalId;
    worldGeneration_ = worldGeneration;
    pointers_ = pointers;
    ownsSpawnedVehicle_ = ownsSpawnedVehicle;
    destroyPending_ = false;
}

void RivalRuntimeHandle::markDestroyPending() {
    if (bound()) {
        destroyPending_ = true;
    }
}

void RivalRuntimeHandle::invalidate() {
    rivalId_ = 0;
    worldGeneration_ = 0;
    pointers_ = {};
    ownsSpawnedVehicle_ = false;
    destroyPending_ = false;
}

bool RivalRuntimeHandle::bound() const {
    return
        rivalId_ != 0 &&
        worldGeneration_ != 0 &&
        pointers_.iVehicle != 0 &&
        pointers_.pVehicle != 0;
}

bool RivalRuntimeHandle::validForGeneration(
    std::uint64_t worldGeneration
) const {
    return
        bound() &&
        worldGeneration_ == worldGeneration &&
        !destroyPending_;
}

bool RivalRuntimeHandle::destroyPending() const {
    return destroyPending_;
}

bool RivalRuntimeHandle::ownsSpawnedVehicle() const {
    return ownsSpawnedVehicle_;
}

std::uint64_t RivalRuntimeHandle::rivalId() const {
    return rivalId_;
}

std::uint64_t RivalRuntimeHandle::worldGeneration() const {
    return worldGeneration_;
}

RivalRuntimePointers RivalRuntimeHandle::pointers() const {
    return pointers_;
}

} // namespace frr::domain
