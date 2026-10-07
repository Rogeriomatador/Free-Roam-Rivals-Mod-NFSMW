#pragma once
#include <cstddef>
#include <cstdint>
#include <span>

namespace frr::domain {
enum class GameplayLoopMatch { Missing, Unique, Ambiguous, UnexpectedTarget };
struct GameplayLoopResolution {
    GameplayLoopMatch match = GameplayLoopMatch::Missing;
    std::uintptr_t callSite = 0;
    std::uintptr_t target = 0;
};
// WFP MW05 main-loop CALL, one cdecl float argument, matched against the
// independently pinned SDK GameFrameTick RVA. Redirected calls fail closed.
GameplayLoopResolution resolveGameplayLoop(std::span<const std::uint8_t> code,
    std::uintptr_t codeAddress, std::uintptr_t imageBase, std::size_t imageSize);
} // namespace frr::domain
