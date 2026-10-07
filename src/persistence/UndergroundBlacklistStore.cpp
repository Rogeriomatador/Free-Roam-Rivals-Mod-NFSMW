#include "UndergroundBlacklistStore.h"

#include "../core/ProfileKey.h"

#include <windows.h>

#include <algorithm>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <limits>
#include <sstream>
#include <string>
#include <system_error>
#include <utility>
#include <vector>

namespace frr::persistence {
namespace {

constexpr std::uint64_t kSchemaVersion = 1;

std::optional<std::uint64_t> readUnsignedField(
    std::string_view json,
    std::string_view key
) {
    const std::string quotedKey =
        std::string("\"") + std::string(key) + "\"";

    const std::size_t keyPos = json.find(quotedKey);
    if (keyPos == std::string_view::npos) {
        return std::nullopt;
    }

    const std::size_t colon =
        json.find(':', keyPos + quotedKey.size());

    if (colon == std::string_view::npos) {
        return std::nullopt;
    }

    std::size_t begin = colon + 1;
    while (begin < json.size() &&
           (json[begin] == ' ' ||
            json[begin] == '\t' ||
            json[begin] == '\r' ||
            json[begin] == '\n')) {
        ++begin;
    }

    std::size_t end = begin;
    while (end < json.size() &&
           json[end] >= '0' &&
           json[end] <= '9') {
        ++end;
    }

    if (end == begin) {
        return std::nullopt;
    }

    std::uint64_t value = 0;
    const char* first = json.data() + begin;
    const char* last = json.data() + end;

    const auto parsed =
        std::from_chars(first, last, value);

    if (parsed.ec != std::errc{} ||
        parsed.ptr != last) {
        return std::nullopt;
    }

    return value;
}

bool replaceFileAtomically(
    const std::filesystem::path& temp,
    const std::filesystem::path& target,
    std::string* error
) {
    if (MoveFileExW(
            temp.c_str(),
            target.c_str(),
            MOVEFILE_REPLACE_EXISTING |
            MOVEFILE_WRITE_THROUGH) != 0) {
        return true;
    }

    if (error) {
        std::ostringstream out;
        out << "MoveFileExW failed with error "
            << GetLastError();
        *error = out.str();
    }

    std::error_code ignored;
    std::filesystem::remove(temp, ignored);
    return false;
}

} // namespace

std::string serializeUndergroundBlacklistProgress(
    const frr::domain::UndergroundBlacklistProgress& progress
) {
    std::ostringstream out;
    out
        << "{\n"
        << "  \"schema\": " << kSchemaVersion << ",\n"
        << "  \"streetRep\": "
        << std::max(progress.streetRep, 0) << ",\n"
        << "  \"qualifierWinsCurrentRank\": "
        << std::max(progress.qualifierWinsCurrentRank, 0)
        << ",\n"
        << "  \"pinkSlipWins\": "
        << std::max(progress.pinkSlipWins, 0) << ",\n"
        << "  \"defeatedMask\": "
        << progress.defeatedMask << ",\n"
        << "  \"discoveredMask\": "
        << progress.discoveredMask << "\n"
        << "}\n";

    return out.str();
}

std::optional<frr::domain::UndergroundBlacklistProgress>
parseUndergroundBlacklistProgress(
    std::string_view json
) {
    const auto schema =
        readUnsignedField(json, "schema");
    const auto streetRep =
        readUnsignedField(json, "streetRep");
    const auto qualifierWins =
        readUnsignedField(
            json,
            "qualifierWinsCurrentRank"
        );
    const auto pinkSlipWins =
        readUnsignedField(json, "pinkSlipWins");
    const auto defeatedMask =
        readUnsignedField(json, "defeatedMask");
    const auto discoveredMask =
        readUnsignedField(json, "discoveredMask");

    if (!schema ||
        !streetRep ||
        !qualifierWins ||
        !pinkSlipWins ||
        !defeatedMask ||
        !discoveredMask ||
        *schema != kSchemaVersion ||
        *streetRep >
            static_cast<std::uint64_t>(
                std::numeric_limits<int>::max()) ||
        *qualifierWins >
            static_cast<std::uint64_t>(
                std::numeric_limits<int>::max()) ||
        *pinkSlipWins >
            static_cast<std::uint64_t>(
                std::numeric_limits<int>::max()) ||
        *defeatedMask >
            std::numeric_limits<std::uint32_t>::max() ||
        *discoveredMask >
            std::numeric_limits<std::uint32_t>::max()) {
        return std::nullopt;
    }

    frr::domain::UndergroundBlacklistProgress out{};
    out.streetRep = static_cast<int>(*streetRep);
    out.qualifierWinsCurrentRank =
        static_cast<int>(*qualifierWins);
    out.pinkSlipWins =
        static_cast<int>(*pinkSlipWins);
    out.defeatedMask =
        static_cast<std::uint32_t>(*defeatedMask);
    out.discoveredMask =
        static_cast<std::uint32_t>(*discoveredMask);

    // Runtime-only facts are never trusted from disk.
    out.careerCompleted = false;
    out.currentTargetPresent = false;

    return out;
}

UndergroundBlacklistStore::UndergroundBlacklistStore(
    std::filesystem::path saveDirectory
) : saveDirectory_(std::move(saveDirectory)) {}

std::filesystem::path
UndergroundBlacklistStore::defaultSaveDirectory() {
    std::vector<wchar_t> buffer(32768, L'\0');

    const DWORD length = GetModuleFileNameW(
        nullptr,
        buffer.data(),
        static_cast<DWORD>(buffer.size())
    );

    std::filesystem::path root;

    if (length == 0 || length >= buffer.size()) {
        root = std::filesystem::current_path();
    } else {
        root = std::filesystem::path(
            std::wstring(buffer.data(), length)
        ).parent_path();
    }

    return
        root /
        "scripts" /
        "FreeRoamRivals" /
        "Saves";
}

std::filesystem::path
UndergroundBlacklistStore::pathForProfile(
    std::uint64_t profileKey
) const {
    return saveDirectory_ /
        ("profile_" +
         frr::formatProfileKey(profileKey) +
         ".json");
}

UndergroundBlacklistLoadResult
UndergroundBlacklistStore::load(
    std::uint64_t profileKey
) const {
    UndergroundBlacklistLoadResult result{};

    const auto path = pathForProfile(profileKey);

    std::error_code ec;
    if (!std::filesystem::exists(path, ec)) {
        result.ok = !ec;
        result.found = false;
        if (ec) {
            result.error = ec.message();
        }
        return result;
    }

    std::ifstream input(path, std::ios::binary);
    if (!input) {
        result.error =
            "Could not open persistence file for reading.";
        return result;
    }

    std::ostringstream buffer;
    buffer << input.rdbuf();

    if (!input.good() && !input.eof()) {
        result.error =
            "Could not read persistence file.";
        return result;
    }

    const auto parsed =
        parseUndergroundBlacklistProgress(
            buffer.str()
        );

    if (!parsed) {
        result.error =
            "Persistence file is malformed or uses an unsupported schema.";
        return result;
    }

    result.ok = true;
    result.found = true;
    result.progress = *parsed;
    return result;
}

bool UndergroundBlacklistStore::save(
    std::uint64_t profileKey,
    const frr::domain::UndergroundBlacklistProgress& progress,
    std::string* error
) const {
    std::error_code ec;
    std::filesystem::create_directories(
        saveDirectory_,
        ec
    );

    if (ec) {
        if (error) {
            *error = ec.message();
        }
        return false;
    }

    const auto target = pathForProfile(profileKey);
    auto temp = target;
    temp += ".tmp";

    {
        std::ofstream output(
            temp,
            std::ios::binary |
            std::ios::trunc
        );

        if (!output) {
            if (error) {
                *error =
                    "Could not open temporary persistence file.";
            }
            return false;
        }

        output << serializeUndergroundBlacklistProgress(
            progress
        );

        output.flush();

        if (!output) {
            if (error) {
                *error =
                    "Could not flush temporary persistence file.";
            }
            output.close();
            std::error_code ignored;
            std::filesystem::remove(temp, ignored);
            return false;
        }
    }

    return replaceFileAtomically(
        temp,
        target,
        error
    );
}

} // namespace frr::persistence
