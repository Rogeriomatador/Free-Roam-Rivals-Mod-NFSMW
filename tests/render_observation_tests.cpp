#include "domain/RenderObservation.h"
#include <array>
#include <cstring>
#include <cstdlib>
#include <iostream>
#include <vector>
using namespace frr::domain;
void require(bool ok, const char* message) {
    if (!ok) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
std::array<std::uint8_t, 17> pattern(std::uint32_t address) {
    std::array<std::uint8_t, 17> p{0xA1,0,0,0,0,0x8B,0x08,0x68,0,0,0,0,0x50,0xFF,0x51,0x40,0xA1};
    std::memcpy(p.data()+1, &address, sizeof(address));
    return p;
}
int main() {
    RenderSignalRouter r;
    require(!r.accept(RenderSignal::Present, 0, 0), "missing device cannot drive sampling");
    require(r.accept(RenderSignal::Present, 1, 10), "Present delivers when EndScene is absent");
    require(r.accept(RenderSignal::EndScene, 1, 20), "EndScene becomes primary");
    require(!r.accept(RenderSignal::Present, 1, 20), "same-frame Present is not a second sample");
    require(!r.accept(RenderSignal::Present, 1, 520), "inclusive lease suppresses fallback");
    require(r.accept(RenderSignal::Present, 1, 521), "silent EndScene releases fallback");
    require(!r.accept(RenderSignal::Present, 1, 19), "regressed clock cannot double sample");
    require(r.accept(RenderSignal::Present, 2, 21), "new device discards old EndScene lease");
    require(r.accept(RenderSignal::EndScene, 2, 22), "EndScene can recover after fallback");
    require(!r.accept(RenderSignal::Present, 2, 23), "recovered EndScene suppresses fallback");
    const auto p = pattern(0x401000);
    auto found = resolveRenderDeviceGlobal(p, 0x400000, 0x2000);
    require(found.match == DeviceGlobalMatch::Unique && found.address == 0x401000, "verified Reset shape resolves global");
    for (int i : {0,5,6,7,12,13,14,15,16}) {
        auto bad = p; bad[i] ^= 1;
        require(resolveRenderDeviceGlobal(bad, 0x400000, 0x2000).match == DeviceGlobalMatch::Missing,
                "wrong opcode, register or Reset slot cannot discover global");
    }
    require(resolveRenderDeviceGlobal(pattern(0x3fffff), 0x400000, 0x2000).match == DeviceGlobalMatch::Missing,
            "global before image rejected");
    require(resolveRenderDeviceGlobal(pattern(0x401ffd), 0x400000, 0x2000).match == DeviceGlobalMatch::Missing,
            "whole pointer must fit image");
    std::vector<std::uint8_t> multiple(p.begin(), p.end());
    multiple.insert(multiple.end(), p.begin(), p.end());
    require(resolveRenderDeviceGlobal(multiple, 0x400000, 0x2000).match == DeviceGlobalMatch::Unique,
            "multiple call sites for same global are consistent");
    const auto other = pattern(0x401004);
    multiple.insert(multiple.end(), other.begin(), other.end());
    found = resolveRenderDeviceGlobal(multiple, 0x400000, 0x2000);
    require(found.match == DeviceGlobalMatch::Ambiguous && found.address == 0,
            "conflicting globals fail closed");
    require(resolveRenderDeviceGlobal(std::span(p).first(16), 0x400000, 0x2000).match == DeviceGlobalMatch::Missing,
            "truncated signature is not read past boundary");
    std::cout << "Render observation tests passed\n";
}
