#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>

namespace frr {

std::optional<std::uint64_t> profileKeyFromName(
    const char* profileName,
    std::size_t capacity,
    bool profileNamed
);

std::string formatProfileKey(std::uint64_t key);

} // namespace frr
