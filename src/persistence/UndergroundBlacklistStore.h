#pragma once

#include "../domain/UndergroundBlacklist.h"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace frr::persistence {

struct UndergroundBlacklistLoadResult {
    bool ok = false;
    bool found = false;
    frr::domain::UndergroundBlacklistProgress progress{};
    std::string error;
};

std::string serializeUndergroundBlacklistProgress(
    const frr::domain::UndergroundBlacklistProgress& progress
);

std::optional<frr::domain::UndergroundBlacklistProgress>
parseUndergroundBlacklistProgress(
    std::string_view json
);

class UndergroundBlacklistStore {
public:
    explicit UndergroundBlacklistStore(
        std::filesystem::path saveDirectory
    );

    static std::filesystem::path defaultSaveDirectory();

    std::filesystem::path pathForProfile(
        std::uint64_t profileKey
    ) const;

    UndergroundBlacklistLoadResult load(
        std::uint64_t profileKey
    ) const;

    bool save(
        std::uint64_t profileKey,
        const frr::domain::UndergroundBlacklistProgress& progress,
        std::string* error = nullptr
    ) const;

private:
    std::filesystem::path saveDirectory_;
};

} // namespace frr::persistence
