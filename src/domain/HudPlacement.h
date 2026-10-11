#pragma once
#include <algorithm>
#include <cmath>
namespace frr::domain {
struct HudPlacement {
    bool radarVisible=false;
    float scale=1,radarX=0,radarY=0,routeX=0,routeY=0,panelX=0,panelY=0;
};
// Screen-relative coordinates; apply D3DVIEWPORT9.X/Y once at drawing.
// Adaptive for 4:3, 16:9, ultrawide, fixed HUD, Xbox-looking HUD packs,
// and WidescreenFix. Pure pixel layout; does not inspect or alter game hooks.
inline HudPlacement placeRivalHud(float width,float height,int radarPosition,
                                 float configuredScale) {
    HudPlacement o{};
    if(!std::isfinite(width)||!std::isfinite(height)||width<360||height<200) return o;
    const float requested=(std::isfinite(configuredScale)&&configuredScale>=0.5f&&
        configuredScale<=1.5f)?configuredScale:1.0f;
    o.scale=std::clamp(std::min(width/1280.0f,height/720.0f)*requested,0.60f,1.55f);
    o.panelX=12; o.panelY=12;
    const float margin=70*o.scale;
    const float right=width-margin, left=margin;
    const float top=150*o.scale,bottom=height-118*o.scale;
    o.radarVisible=radarPosition>=0&&radarPosition<=2;
    if(radarPosition==0) {o.radarX=left;o.radarY=bottom;}
    else if(radarPosition==1) {o.radarX=right;o.radarY=top;}
    else {o.radarX=right;o.radarY=height*0.54f;}
    o.radarX=std::clamp(o.radarX,margin,std::max(margin,width-margin));
    o.radarY=std::clamp(o.radarY,margin,std::max(margin,height-margin));
    o.routeX=width*0.5f;
    o.routeY=std::clamp(height*0.27f,120*o.scale,height-margin);
    return o;
}
}
