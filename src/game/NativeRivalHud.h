#pragma once
#include <cstdint>
namespace frr::game {
struct NativeRivalHudView {
    bool visible=false,playerLeading=false,challengeReady=false;
    float holdProgress=0;
    // Drawn only in FRR's separately labeled overlay next to the minimap.
    // NOT injected into vanilla TRACK_MAP or native GPS/FNG objects.
    bool rivalBeaconValid=false,routeCueValid=false,rivalStationary=false;
    float rivalFront=0,rivalRight=0,rivalMeters=0;
    float routeFront=0,routeRight=0,routeMeters=0;
    char lines[6][64]{};
};
void configureNativeRivalHud(bool enabled,int radarPosition=2,float radarScale=1.0f);
void publishNativeRivalHud(const NativeRivalHudView& view);
// Called ONLY before native EndScene, inside the existing scene. Reads mod
// state only; restores the full D3D state and keeps no reset-sensitive resource.
void renderNativeRivalHud(void* device);
}
