#pragma once
#include <cstddef>
#include <cstdint>

namespace frr::domain {
inline std::uint64_t nativeCodeFingerprint(const unsigned char* bytes, std::size_t size) {
    std::uint64_t hash=14695981039346656037ull;
    for (std::size_t i=0;i<size;++i) hash=(hash^bytes[i])*1099511628211ull;
    return hash;
}
inline std::size_t firstNativeCodeDifference(const unsigned char* a, const unsigned char* b, std::size_t size) {
    for (std::size_t i=0;i<size;++i) if (a[i]!=b[i]) return i;
    return size;
}
// Classifies entry bytes only. A jump shape is NOT proof of a foreign hook.
enum class NativeEntryJump { None, RelativeE9, IndirectFF25 };
inline NativeEntryJump nativeEntryJump(const unsigned char* bytes, std::size_t size) {
    if (size>=5 && bytes[0]==0xE9) return NativeEntryJump::RelativeE9;
    if (size>=6 && bytes[0]==0xFF && bytes[1]==0x25) return NativeEntryJump::IndirectFF25;
    return NativeEntryJump::None;
}
// Exact supported PE .text: RVA/raw start 0x1000, raw end 0x490000.
// Used only AFTER the full executable version/MD5 check, never for other PEs.
inline bool supportedNativeCodeFileOffset(std::uint32_t address, std::size_t size, std::uint32_t& offset) {
    if (address<0x401000u || address>=0x890000u || size>0x890000u-address) return false;
    offset=address-0x400000u;
    return true;
}
}
