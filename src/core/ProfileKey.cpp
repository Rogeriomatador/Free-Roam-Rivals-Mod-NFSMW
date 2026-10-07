#include "ProfileKey.h"

#include <iomanip>
#include <sstream>
#include <string_view>

namespace frr {
namespace {

constexpr std::uint64_t kFnvOffset = 14695981039346656037ull;
constexpr std::uint64_t kFnvPrime = 1099511628211ull;
constexpr std::string_view kNamespace = "FreeRoamRivals/Profile/v1|";

void mixByte(std::uint64_t& hash, unsigned char value) {
    hash ^= static_cast<std::uint64_t>(value);
    hash *= kFnvPrime;
}

} // namespace

std::optional<std::uint64_t> profileKeyFromName(
    const char* profileName,
    std::size_t capacity,
    bool profileNamed
) {
    if (!profileNamed || !profileName || capacity == 0) {
        return std::nullopt;
    }

    std::size_t length = 0;
    while (length < capacity && profileName[length] != '\0') {
        ++length;
    }

    if (length == 0 || length == capacity) {
        return std::nullopt;
    }

    std::uint64_t hash = kFnvOffset;

    for (const char ch : kNamespace) {
        mixByte(hash, static_cast<unsigned char>(ch));
    }

    for (std::size_t i = 0; i < length; ++i) {
        mixByte(
            hash,
            static_cast<unsigned char>(profileName[i])
        );
    }

    return hash;
}

std::string formatProfileKey(std::uint64_t key) {
    std::ostringstream out;
    out << std::hex
        << std::setfill('0')
        << std::setw(16)
        << key;
    return out.str();
}

} // namespace frr
