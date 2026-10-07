#include "RenderObservation.h"
#include <cstring>

namespace frr::domain {
bool RenderSignalRouter::accept(RenderSignal signal, std::uintptr_t device, std::uint64_t nowMillis) {
    if (device == 0) return false;
    if (device != device_) {
        device_ = device;
        haveEndScene_ = false;
    }
    if (signal == RenderSignal::EndScene) {
        haveEndScene_ = true;
        lastEndSceneMillis_ = nowMillis;
        return true;
    }
    // Clock regression also suppresses fallback rather than double-sampling.
    return !haveEndScene_ || (nowMillis >= lastEndSceneMillis_ &&
        nowMillis - lastEndSceneMillis_ > 500);
}

DeviceGlobalResolution resolveRenderDeviceGlobal(
    std::span<const std::uint8_t> code, std::uintptr_t imageBase, std::size_t imageSize) {
    DeviceGlobalResolution out{};
    if (code.size() < 17 || imageSize < sizeof(std::uint32_t)) return out;
    for (std::size_t i = 0; i <= code.size() - 17; ++i) {
        const auto* p = code.data() + i;
        if (p[0] != 0xA1 || p[5] != 0x8B || p[6] != 0x08 || p[7] != 0x68 ||
            p[12] != 0x50 || p[13] != 0xFF || p[14] != 0x51 || p[15] != 0x40 || p[16] != 0xA1) continue;
        std::uint32_t address = 0;
        std::memcpy(&address, p + 1, sizeof(address));
        if (address < imageBase || address - imageBase > imageSize - sizeof(address)) continue;
        if (out.match == DeviceGlobalMatch::Unique && out.address != address) {
            return {DeviceGlobalMatch::Ambiguous, 0};
        }
        out = {DeviceGlobalMatch::Unique, address};
    }
    return out;
}
} // namespace frr::domain
