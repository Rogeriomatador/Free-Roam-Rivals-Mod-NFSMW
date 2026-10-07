#pragma once

#include <cstdint>

namespace frr::game {

struct ChallengeInputProbeConfig {
    unsigned fallbackVirtualKey = 0x47u; // 'G'
};

class ChallengeInputProbe {
public:
    static void configure(
        const ChallengeInputProbeConfig& config
    );

    // Called from the verified game input-poll callback after the engine has
    // refreshed its own bindings. This probe reads only the configured
    // fallback virtual key and never injects input.
    static void onPoll();

    // Future EncounterDirector integration consumes exactly one queued edge.
    static bool consumePress();

    static std::uint64_t totalPresses();
    static unsigned fallbackVirtualKey();
};

} // namespace frr::game
