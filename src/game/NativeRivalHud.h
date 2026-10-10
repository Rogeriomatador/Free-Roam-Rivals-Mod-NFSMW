#pragma once
#include <cstdint>
namespace frr::game {
struct NativeRivalHudView {
    bool visible=false,playerLeading=false;
    float holdProgress=0;
    char lines[6][64]{};
};
void configureNativeRivalHud(bool enabled);
void publishNativeRivalHud(const NativeRivalHudView& view);
// Called ONLY before native EndScene, inside the existing scene. Reads mod
// state only; restores the full D3D state and keeps no reset-sensitive resource.
void renderNativeRivalHud(void* device);
}
