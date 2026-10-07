#include "domain/GameplayLoopEvidence.h"
#include "domain/MutationReadiness.h"
#include <vector>
#include <cstring>
#include <cstdlib>
#include <iostream>

void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
int main() {
    using namespace frr::domain;
    constexpr std::uintptr_t base = 0x400000, codeAt = 0x600000;
    std::vector<std::uint8_t> code{0xE8,0,0,0,0,0xA0,0,0,0,0,0x83,0xC4,4,0x84,0xC0,0x74,8,0xC6,5};
    const std::int32_t relative = 0x663D30 - static_cast<std::int32_t>(codeAt + 5);
    const std::uint32_t global = 0x925E90;
    std::memcpy(code.data()+1, &relative, 4); std::memcpy(code.data()+6, &global, 4);
    auto resolve = [&](const auto& bytes) { return resolveGameplayLoop(bytes, codeAt, base, 0x600000); };
    auto found = resolve(code);
    require(found.match == GameplayLoopMatch::Unique && found.target == 0x663D30 && found.callSite == codeAt,
        "CALL ABI and pinned target agree");
    require(classifyGameplayLoopRoute(found.match, false) == GameplayLoopRoute::DirectPinnedTarget,
        "direct pinned target is authorized without a chain owner");

    auto backward = code;
    const std::int32_t negative = 0x663D30 - 0x700005;
    std::memcpy(backward.data()+1, &negative, 4);
    require(resolveGameplayLoop(backward, 0x700000, base, 0x600000).match == GameplayLoopMatch::Unique,
        "signed backward rel32 destination resolves correctly");
    require(resolveGameplayLoop(code, codeAt, base, 0x200010).match == GameplayLoopMatch::Missing,
        "section span cannot exceed image bounds");
    for (unsigned offset : {0,5,10,11,12,13,14,15,17,18}) {
        auto corrupt = code; corrupt[offset] ^= 1;
        require(resolve(corrupt).match != GameplayLoopMatch::Unique, "every ABI opcode/cleanup byte required");
    }
    auto changed = code; changed[1] ^= 1;
    require(resolve(changed).match == GameplayLoopMatch::UnexpectedTarget, "redirected CALL cannot authorize guessed ABI");
    require(classifyGameplayLoopRoute(GameplayLoopMatch::UnexpectedTarget, false) == GameplayLoopRoute::Blocked,
        "unknown redirected owner remains blocked");
    require(classifyGameplayLoopRoute(GameplayLoopMatch::UnexpectedTarget, true) == GameplayLoopRoute::KnownChainedWrapper,
        "explicitly recognized redirected owner may preserve a known chain");
    require(classifyGameplayLoopRoute(GameplayLoopMatch::Ambiguous, true) == GameplayLoopRoute::Blocked,
        "known owner cannot override ambiguous call-site evidence");

    changed = code; std::uint32_t invalidGlobal = base-1; std::memcpy(changed.data()+6, &invalidGlobal, 4);
    require(resolve(changed).match == GameplayLoopMatch::Missing, "global before image rejected");
    invalidGlobal = base + 0x600000; std::memcpy(changed.data()+6, &invalidGlobal, 4);
    require(resolve(changed).match == GameplayLoopMatch::Missing, "global at image end rejected");
    changed = code; changed.insert(changed.end(), code.begin(), code.end());
    require(resolve(changed).match == GameplayLoopMatch::Ambiguous, "multiple call sites fail closed");
    code.pop_back(); require(resolve(code).match == GameplayLoopMatch::Missing, "truncated pattern rejected");
    require(resolveGameplayLoop(code, base-1, base, 0x600000).match == GameplayLoopMatch::Missing, "section outside image rejected");

    MutationReadinessInput input{};
    input.frameTickProbeEnabled = input.frameTickProbeInstalled = true;
    input.frameTickCount = 1; input.frameTickThreadId = 77;
    input.gameplayLoopSourceVerified = input.gameplayLoopThreadConsistent = true;
    input.gameplayLoopCompletedCount = 1; input.gameplayLoopThreadId = 77;
    auto report = evaluateMutationReadiness(input);
    require(report.gameplayThreadConfirmed && !report.readyForConstructionExperiment && report.blocker == MutationReadinessBlocker::FreeRoamNotObserved,
        "verified completed loop accepts thread evidence but no other spawn gate");
    for (int i=0; i<4; ++i) {
        auto invalid = input;
        if(i==0) invalid.gameplayLoopSourceVerified=false;
        if(i==1) invalid.gameplayLoopThreadConsistent=false;
        if(i==2) invalid.gameplayLoopCompletedCount=0;
        if(i==3) invalid.gameplayLoopThreadId=0;
        require(!evaluateMutationReadiness(invalid).gameplayThreadConfirmed, "missing source/delivery/thread evidence blocks loop path");
    }
    input.gameplayLoopThreadId=78;
    require(evaluateMutationReadiness(input).blocker == MutationReadinessBlocker::MainLoopThreadUnconfirmed, "thread mismatch blocks gameplay path");
    input.gameplayLoopThreadId=77; input.frameTickProbeEnabled=false;
    require(evaluateMutationReadiness(input).blocker == MutationReadinessBlocker::FrameTickProbeDisabled, "diagnostic opt-in still required");
    std::cout << "Gameplay-loop evidence tests passed\n";
}
