#pragma once
#include <cstdint>
#include <span>

namespace frr::domain {
enum class RenderSignal { EndScene, Present };

// Value-only routing. Present is a fallback, never a second sample for a
// recently observed EndScene. Changing devices discards the previous lease.
class RenderSignalRouter {
public:
    bool accept(RenderSignal signal, std::uintptr_t device, std::uint64_t nowMillis);
private:
    std::uintptr_t device_ = 0;
    bool haveEndScene_ = false;
    std::uint64_t lastEndSceneMillis_ = 0;
};

enum class DeviceGlobalMatch { Missing, Unique, Ambiguous };
struct DeviceGlobalResolution {
    DeviceGlobalMatch match = DeviceGlobalMatch::Missing;
    std::uintptr_t address = 0;
};

// MW05 Reset call-site pattern independently used by WidescreenFixesPack.
// The loaded global must belong to the guarded executable image.
DeviceGlobalResolution resolveRenderDeviceGlobal(
    std::span<const std::uint8_t> code, std::uintptr_t imageBase, std::size_t imageSize);
} // namespace frr::domain
