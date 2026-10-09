#include "domain/NativeFactorySafety.h"
#include "domain/NativeCodeCompatibility.h"
#include "domain/NativeConstructionConfirmation.h"
#include "domain/NativeRegistrySafety.h"
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
    std::uint64_t frame=0; unsigned confirmations=0;
    auto step=[&](std::uint64_t f,bool registry,bool member,bool pursuit) {
        return advanceNativeConstructionConfirmation(f,registry,member,pursuit,frame,confirmations);
    };
    if (step(1,true,true,true)!=NativeConstructionDecision::Wait || confirmations!=1) return 13;
    if (step(1,true,true,true)!=NativeConstructionDecision::Wait || confirmations!=1) return 14;
    if (step(2,true,false,true)!=NativeConstructionDecision::Wait || confirmations) return 15;
    if (step(3,true,true,true)!=NativeConstructionDecision::Wait || confirmations!=1) return 16;
    if (step(4,false,true,true)!=NativeConstructionDecision::Wait || confirmations) return 17;
    if (step(5,true,true,true)!=NativeConstructionDecision::Wait) return 18;
    if (step(6,true,true,false)!=NativeConstructionDecision::Wait || confirmations) return 19;
    if (step(7,true,true,true)!=NativeConstructionDecision::Wait) return 20;
    if (step(8,true,true,true)!=NativeConstructionDecision::Confirm || confirmations!=2) return 21;
    if (step(7,true,true,true)!=NativeConstructionDecision::Wait || confirmations!=2) return 22;
    // Regression: an unrelated traffic car departs between completed frames.
    // The synchronous constructor still must preserve the existing fleet;
    // later ownership confirmation requires the player and new allocation.
    const std::uintptr_t beforeIV[]{0x100,0x200},beforeP[]{0x1000,0x2000};
    const std::uintptr_t createdIV[]{0x300,0x200,0x100},createdP[]{0x3000,0x1000,0x2000};
    const std::uintptr_t streamedIV[]{0x400,0x300,0x100},streamedP[]{0x4000,0x1000,0x3000};
    const NativeRegistryView baseline{beforeIV,2,beforeP,2};
    const NativeRegistryView constructed{createdIV,3,createdP,3};
    const NativeRegistryView streamed{streamedIV,3,streamedP,3};
    if (!nativeRegistryPreserved(baseline,constructed)) return 23;
    if (nativeRegistryPreserved(baseline,streamed)) return 24; // Still reject synchronous eviction.
    frame=0;confirmations=0;
    if (step(100,true,nativeConstructionMembership(constructed,0x300,0x3000,0x100,0x1000),true)
        !=NativeConstructionDecision::Wait || confirmations!=1) return 25;
    if (step(101,true,nativeConstructionMembership(streamed,0x300,0x3000,0x100,0x1000),true)
        !=NativeConstructionDecision::Confirm || confirmations!=2) return 26;
    // Every critical interface must be freshly registered. Lose each one,
    // reset the streak and require two subsequent safe completed frames.
    for (unsigned missing=0;missing<4;++missing) {
        std::uintptr_t live[]{0x100,0x300},physical[]{0x1000,0x3000};
        if (missing<2) live[missing]=0x777; else physical[missing-2]=0x888;
        const NativeRegistryView lost{live,2,physical,2};
        if (nativeConstructionMembership(lost,0x300,0x3000,0x100,0x1000)) return 27;
        frame=0;confirmations=0;
        step(200,true,true,true);
        if (step(201,true,nativeConstructionMembership(lost,0x300,0x3000,0x100,0x1000),true)
            !=NativeConstructionDecision::Wait || confirmations) return 28;
        if (step(202,true,true,true)!=NativeConstructionDecision::Wait || confirmations!=1) return 29;
        if (step(203,true,true,true)!=NativeConstructionDecision::Confirm || confirmations!=2) return 30;
    }
    if (nativeConstructionMembership(constructed,0x100,0x1000,0x100,0x1000) ||
        nativeConstructionMembership(constructed,0,0x3000,0x100,0x1000) ||
        nativeRegistryContains(nullptr,0,0x100) || nativeRegistryContains(createdIV,3,0)) return 31;
    std::puts("Native factory safety: capacity, code signatures, PE offsets, frame confirmation, unrelated fleet turnover and critical membership passed; no engine calls");
    return 0;
}
