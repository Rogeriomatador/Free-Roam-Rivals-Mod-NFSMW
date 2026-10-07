#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace frr::domain {
enum class GameplayLoopMatch { Missing, Unique, Ambiguous, UnexpectedTarget };

enum class GameplayLoopRoute {
    Blocked,
    DirectPinnedTarget,
    KnownChainedWrapper
};

struct GameplayLoopResolution {
    GameplayLoopMatch match = GameplayLoopMatch::Missing;
    std::uintptr_t callSite = 0;
    std::uintptr_t target = 0;
};

// WFP MW05 main-loop CALL, one cdecl float argument, matched against the
// independently pinned SDK GameFrameTick RVA. Redirected calls are classified
// separately so runtime code can fail closed unless the current chain owner is
// explicitly recognized.
GameplayLoopResolution resolveGameplayLoop(std::span<const std::uint8_t> code,
    std::uintptr_t codeAddress, std::uintptr_t imageBase, std::size_t imageSize);

// A direct pinned target is always eligible. A redirected target is eligible
// only when runtime inspection has independently identified an explicitly
// supported chain owner. Missing/ambiguous routes never become eligible.
GameplayLoopRoute classifyGameplayLoopRoute(
    GameplayLoopMatch match,
    bool knownChainedWrapper
);
} // namespace frr::domain
