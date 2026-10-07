#pragma once

#include <cstdint>

namespace frr::domain {

struct RivalRuntimePointers {
    std::uintptr_t iVehicle = 0;
    std::uintptr_t pVehicle = 0;
    std::uintptr_t vehicleAI = 0;
};

class RivalRuntimeHandle {
public:
    void bind(
        std::uint64_t rivalId,
        std::uint64_t worldGeneration,
        RivalRuntimePointers pointers,
        bool ownsSpawnedVehicle
    );

    void markDestroyPending();
    void invalidate();

    bool bound() const;
    bool validForGeneration(std::uint64_t worldGeneration) const;
    bool destroyPending() const;
    bool ownsSpawnedVehicle() const;

    std::uint64_t rivalId() const;
    std::uint64_t worldGeneration() const;
    RivalRuntimePointers pointers() const;

private:
    std::uint64_t rivalId_ = 0;
    std::uint64_t worldGeneration_ = 0;
    RivalRuntimePointers pointers_{};
    bool ownsSpawnedVehicle_ = false;
    bool destroyPending_ = false;
};

} // namespace frr::domain
