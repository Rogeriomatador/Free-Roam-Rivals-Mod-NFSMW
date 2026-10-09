#pragma once
#include <cstdint>

namespace frr::domain {
enum class NativeConstructionDecision { Wait, Confirm };
// Require two distinct completed gameplay frames. Losing membership or clear
// pursuit resets the streak; repeating one frame cannot confirm ownership.
inline NativeConstructionDecision advanceNativeConstructionConfirmation(
    std::uint64_t frame, bool registryReadable, bool bothMembership, bool pursuitClear,
    std::uint64_t& lastFrame, unsigned& confirmations) {
    if (!registryReadable || !bothMembership || !pursuitClear || !frame) {
        confirmations=0; lastFrame=frame;
        return NativeConstructionDecision::Wait;
    }
    if (frame<=lastFrame) return NativeConstructionDecision::Wait;
    lastFrame=frame;
    if (confirmations<2) ++confirmations;
    return confirmations>=2 ? NativeConstructionDecision::Confirm : NativeConstructionDecision::Wait;
}
}
