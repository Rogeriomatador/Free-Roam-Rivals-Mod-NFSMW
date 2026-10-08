#include "domain/NativeFactorySafety.h"
#include "domain/NativeCodeCompatibility.h"
#include <array>
#include <cstdio>
#include <initializer_list>
#include <limits>

int main() {
    using frr::domain::nativeFactoryCapacitySafe;
    // Exhaust both native thresholds, including equality (the target returns
    // false there), overflow values and unreadable counters.
    for (bool extended : {false, true}) {
        const unsigned limit = extended ? 64u : 52u;
        for (unsigned count = 0; count <= 128; ++count) {
            if (nativeFactoryCapacitySafe(true, count, extended) != (count < limit)) return 1;
            if (nativeFactoryCapacitySafe(false, count, extended)) return 2;
        }
        if (nativeFactoryCapacitySafe(true, std::numeric_limits<unsigned>::max(), extended)) return 3;
    }
    using namespace frr::domain;
    const unsigned char hello[]{'h','e','l','l','o'};
    if (nativeCodeFingerprint(hello,5)!=0xa430d84680aabd0bull) return 4;
    if (nativeCodeFingerprint(hello,0)!=14695981039346656037ull) return 5;
    std::array<unsigned char,256> a{},b{};
    if (firstNativeCodeDifference(a.data(),b.data(),256)!=256) return 6;
    for (std::size_t i : {std::size_t(0),std::size_t(128),std::size_t(255)}) {
        b[i]=1;
        if (firstNativeCodeDifference(a.data(),b.data(),256)!=i) return 7;
        if (nativeCodeFingerprint(a.data(),256)==nativeCodeFingerprint(b.data(),256)) return 8;
        b[i]=0;
    }
    const unsigned char e9[]{0xE9,0,0,0,0};
    const unsigned char ff25[]{0xFF,0x25,0,0,0,0};
    const unsigned char notJump[]{0xFF,0x15,0,0,0,0};
    if (nativeEntryJump(e9,5)!=NativeEntryJump::RelativeE9 ||
        nativeEntryJump(e9,4)!=NativeEntryJump::None ||
        nativeEntryJump(ff25,6)!=NativeEntryJump::IndirectFF25 ||
        nativeEntryJump(ff25,5)!=NativeEntryJump::None ||
        nativeEntryJump(notJump,6)!=NativeEntryJump::None) return 9;
    std::uint32_t offset=0;
    if (!supportedNativeCodeFileOffset(0x422480,256,offset) || offset!=0x22480) return 10;
    if (!supportedNativeCodeFileOffset(0x88FF00,256,offset) || offset!=0x48FF00) return 11;
    if (supportedNativeCodeFileOffset(0x400FFF,256,offset) ||
        supportedNativeCodeFileOffset(0x88FF01,256,offset) ||
        supportedNativeCodeFileOffset(0x890000,1,offset) ||
        supportedNativeCodeFileOffset(0xFFFFFFFF,256,offset) ||
        supportedNativeCodeFileOffset(0x422480,std::numeric_limits<std::size_t>::max(),offset)) return 12;
    std::puts("Native factory capacity guard: capacity boundaries, signature/difference/entry-shape and exact PE text offset tests passed; no engine calls");
    return 0;
}
