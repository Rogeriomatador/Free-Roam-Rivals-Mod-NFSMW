#include "game/GameplayLoopHook.h"
#include <windows.h>
#include <cstring>
#include <cstdlib>
#include <iostream>

namespace {
using Fn = void (__cdecl*)(float);
LONG originals=0, beforeCalls=0, afterCalls=0;
std::uint32_t argument=0;
bool recurse=false, recurseAfter=false;
Fn volatile cached=nullptr;
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
__declspec(noinline) void __cdecl engine(float value) {
    require(!frr::game::GameplayLoopHook::isInAfterCallback(), "engine and recursive engine never receive post-callback permit");
    InterlockedIncrement(&originals);
    std::memcpy(&argument, &value, 4);
    if (recurse) { recurse=false; cached(value); }
}
void before(float) {
    require(!frr::game::GameplayLoopHook::isInAfterCallback(), "pre callback cannot mutate through post-callback permit");
    if (beforeCalls==0) require(originals==0, "first entry observation precedes original");
    InterlockedIncrement(&beforeCalls);
}
void after(float value) {
    require(frr::game::GameplayLoopHook::isInAfterCallback(), "outer post callback has current boundary permit");
    std::uint32_t bits=0; std::memcpy(&bits, &value, 4);
    require(originals>0 && argument==bits, "original completes with unchanged argument before post callback");
    InterlockedIncrement(&afterCalls);
    if (recurseAfter) { recurseAfter=false; cached(value); }
}
DWORD WINAPI otherThread(void*) { cached(0.5f); return 0; }
}
int main() {
    using frr::game::GameplayLoopHook;
    cached=&engine;
    require(!GameplayLoopHook::attachForTest(reinterpret_cast<void*>(1),before,after), "unreadable target rejected");
    require(GameplayLoopHook::attachForTest(reinterpret_cast<void*>(&engine),before,after), "native typed loop hook installs");
    require(!GameplayLoopHook::snapshot().sourceVerified, "test attach cannot fabricate runtime signature provenance");
    cached(0.125f);
    require(originals==1 && beforeCalls==1 && afterCalls==1 && argument==0x3E000000u, "cached entry calls original and both boundaries");
    cached(-0.0f);
    require(argument==0x80000000u, "float sign bit preserved");
    recurse=true; cached(0.25f);
    require(originals==4 && beforeCalls==3 && afterCalls==3, "recursive original chains fully with only outer callbacks");
    require(!GameplayLoopHook::isInAfterCallback(), "post-callback permit expires on return");
    recurseAfter=true; cached(0.3f);
    require(originals==6 && beforeCalls==4 && afterCalls==4, "reentrant engine from post callback runs without a nested permit");
    auto snapshot=GameplayLoopHook::snapshot();
    require(snapshot.entered==6 && snapshot.completed==6 && snapshot.threadConsistent && snapshot.threadId==GetCurrentThreadId(), "health measures entry/completion and owner thread");
    HANDLE thread=CreateThread(nullptr,0,otherThread,nullptr,0,nullptr);
    require(thread!=nullptr && WaitForSingleObject(thread,5000)==WAIT_OBJECT_0, "second thread completed");
    CloseHandle(thread);
    cached(0.125f);
    require(originals==8 && beforeCalls==4 && afterCalls==4 && !GameplayLoopHook::snapshot().threadConsistent,
        "thread change revokes callbacks permanently while preserving original calls");
    GameplayLoopHook::detachForTest(); cached(0.125f);
    require(originals==9 && afterCalls==4, "cached entry restored after hook removal");
    std::cout << "Native gameplay-loop hook tests passed\n";
}
