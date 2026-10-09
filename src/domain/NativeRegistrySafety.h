#pragma once
#include <cstddef>
#include <cstdint>

namespace frr::domain {
struct NativeRegistryView {
    const std::uintptr_t* live;
    std::size_t liveCount;
    const std::uintptr_t* physical;
    std::size_t physicalCount;
};
inline bool nativeRegistryContains(const std::uintptr_t* entries, std::size_t count,
    std::uintptr_t pointer) {
    if (!entries || !pointer) return false;
    for (std::size_t i=0;i<count;++i) if (entries[i]==pointer) return true;
    return false;
}
// Compare within one synchronous engine operation, before another game update.
// Do not freeze unrelated traffic identities across completed gameplay frames.
inline bool nativeRegistryPreserved(const NativeRegistryView& before,
    const NativeRegistryView& after) {
    for (std::size_t i=0;i<before.liveCount;++i)
        if (!nativeRegistryContains(after.live,after.liveCount,before.live[i])) return false;
    for (std::size_t i=0;i<before.physicalCount;++i)
        if (!nativeRegistryContains(after.physical,after.physicalCount,before.physical[i])) return false;
    return true;
}
// Membership only: callers must still check world/profile generation, vtables,
// simable/handle/model identity and both pursuits before any virtual write.
inline bool nativeConstructionMembership(const NativeRegistryView& current,
    std::uintptr_t ownedIV, std::uintptr_t ownedP,
    std::uintptr_t playerIV, std::uintptr_t playerP) {
    return ownedIV!=playerIV && ownedP!=playerP &&
        nativeRegistryContains(current.live,current.liveCount,ownedIV) &&
        nativeRegistryContains(current.physical,current.physicalCount,ownedP) &&
        nativeRegistryContains(current.live,current.liveCount,playerIV) &&
        nativeRegistryContains(current.physical,current.physicalCount,playerP);
}
}
