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

    // Called after the verified game-loop function returns. Reads only the
    // configured fallback key while this process is foreground; no injection
    // and no claim that the internal native-action mirror was polled.
    static void onPoll();

    // Future EncounterDirector integration consumes exactly one queued edge.
    static bool consumePress();

    static std::uint64_t totalPresses();
    static unsigned fallbackVirtualKey();
};

} // namespace frr::game

