#include "core/ProfileKey.h"
#include "persistence/UndergroundBlacklistStore.h"

#include <windows.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>

namespace {

void require(bool value, const char* message) {
    if (!value) {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(1);
    }
}

} // namespace

int main() {
    using namespace frr;
    using namespace frr::domain;
    using namespace frr::persistence;

    const auto keyA =
        profileKeyFromName("PROFILE_A", 32, true);
    const auto keyA2 =
        profileKeyFromName("PROFILE_A", 32, true);
    const auto keyB =
        profileKeyFromName("PROFILE_B", 32, true);

    require(
        keyA.has_value() &&
        keyA2.has_value() &&
        keyB.has_value(),
        "named profiles produce pseudonymous keys"
    );

    require(
        *keyA == *keyA2,
        "profile key is deterministic"
    );

    require(
        *keyA != *keyB,
        "different profile names use different keys"
    );

    require(
        !profileKeyFromName("", 32, true).has_value(),
        "empty profile name is rejected"
    );

    require(
        !profileKeyFromName(
            "PROFILE_A",
            32,
            false
        ).has_value(),
        "unnamed profile is rejected"
    );

    UndergroundBlacklistProgress original{};
    original.careerCompleted = true;
    original.streetRep = 725;
    original.qualifierWinsCurrentRank = 4;
    original.pinkSlipWins = 2;
    original.defeatedMask = 0x00000300u;
    original.discoveredMask = 0x00000380u;
    original.currentTargetPresent = true;

    const auto json =
        serializeUndergroundBlacklistProgress(
            original
        );

    const auto parsed =
        parseUndergroundBlacklistProgress(json);

    require(parsed.has_value(),
            "serialized progress parses");
    require(parsed->streetRep == 725,
            "street rep round-trips");
    require(parsed->qualifierWinsCurrentRank == 4,
            "qualifier wins round-trip");
    require(parsed->pinkSlipWins == 2,
            "pink slip wins round-trip");
    require(parsed->defeatedMask == 0x00000300u,
            "defeated mask round-trips");
    require(parsed->discoveredMask == 0x00000380u,
            "discovered mask round-trips");
    require(!parsed->careerCompleted,
            "career completion is runtime-owned, not persisted");
    require(!parsed->currentTargetPresent,
            "target presence is runtime-owned, not persisted");

    require(
        !parseUndergroundBlacklistProgress(
            "{\"schema\":999}"
        ).has_value(),
        "unsupported schema fails closed"
    );

    const auto testRoot =
        std::filesystem::temp_directory_path() /
        ("FreeRoamRivalsPersistenceTests_" +
         std::to_string(GetCurrentProcessId()));

    std::error_code ignored;
    std::filesystem::remove_all(testRoot, ignored);

    UndergroundBlacklistStore store(testRoot);

    const auto missing = store.load(*keyA);
    require(
        missing.ok && !missing.found,
        "missing profile store is a clean empty state"
    );

    std::string error;
    require(
        store.save(*keyA, original, &error),
        "atomic mod-side progress save succeeds"
    );

    const auto loaded = store.load(*keyA);
    require(
        loaded.ok && loaded.found,
        "saved profile progress loads"
    );
    require(
        loaded.progress.streetRep == 725 &&
        loaded.progress.defeatedMask == 0x00000300u,
        "loaded persistence matches saved values"
    );

    {
        std::ofstream corrupt(
            store.pathForProfile(*keyA),
            std::ios::binary |
            std::ios::trunc
        );
        corrupt << "{broken";
    }

    const auto malformed = store.load(*keyA);
    require(
        !malformed.ok,
        "malformed persistence fails closed"
    );

    std::filesystem::remove_all(testRoot, ignored);

    std::cout
        << "Free Roam Rivals persistence tests passed.\n";
    return 0;
}
