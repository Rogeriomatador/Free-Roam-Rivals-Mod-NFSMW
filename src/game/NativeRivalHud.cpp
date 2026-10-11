#include "NativeRivalHud.h"
#include <windows.h>
#include <d3d9.h>
#include <array>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <mutex>
#include <vector>

namespace frr::game {
namespace {
std::atomic<bool> enabled{false};
std::mutex mutex;
NativeRivalHudView current{};
std::uint64_t publishedAt=0;
struct Vertex {float x,y,z,rhw;DWORD color;};
// Small original bitmap alphabet; no game fonts, textures, FNG writes or D3DX.
std::array<unsigned,7> glyph(char c) {
    if(c>='a'&&c<='z') c=static_cast<char>(c-'a'+'A');
    switch(c) {
        case 'A':return {14,17,17,31,17,17,17};case 'B':return {30,17,17,30,17,17,30};
        case 'C':return {14,17,16,16,16,17,14};case 'D':return {30,17,17,17,17,17,30};
        case 'E':return {31,16,16,30,16,16,31};case 'F':return {31,16,16,30,16,16,16};
        case 'G':return {14,17,16,23,17,17,15};case 'H':return {17,17,17,31,17,17,17};
        case 'I':return {14,4,4,4,4,4,14};case 'J':return {7,2,2,2,2,18,12};
        case 'K':return {17,18,20,24,20,18,17};case 'L':return {16,16,16,16,16,16,31};
        case 'M':return {17,27,21,21,17,17,17};case 'N':return {17,25,21,19,17,17,17};
        case 'O':return {14,17,17,17,17,17,14};case 'P':return {30,17,17,30,16,16,16};
        case 'Q':return {14,17,17,17,21,18,13};case 'R':return {30,17,17,30,20,18,17};
        case 'S':return {15,16,16,14,1,1,30};case 'T':return {31,4,4,4,4,4,4};
        case 'U':return {17,17,17,17,17,17,14};case 'V':return {17,17,17,17,17,10,4};
        case 'W':return {17,17,17,21,21,21,10};case 'X':return {17,17,10,4,10,17,17};
        case 'Y':return {17,17,10,4,4,4,4};case 'Z':return {31,1,2,4,8,16,31};
        case '0':return {14,17,19,21,25,17,14};case '1':return {4,12,4,4,4,4,14};
        case '2':return {14,17,1,2,4,8,31};case '3':return {30,1,1,14,1,1,30};
        case '4':return {2,6,10,18,31,2,2};case '5':return {31,16,16,30,1,1,30};
        case '6':return {14,16,16,30,17,17,14};case '7':return {31,1,2,4,8,8,8};
        case '8':return {14,17,17,14,17,17,14};case '9':return {14,17,17,15,1,1,14};
        case ':':return {0,4,4,0,4,4,0};case '.':return {0,0,0,0,0,4,4};
        case '-':return {0,0,0,31,0,0,0};case '/':return {1,1,2,4,8,16,16};
        case '[':return {14,8,8,8,8,8,14};case ']':return {14,2,2,2,2,2,14};
        case '+':return {0,4,4,31,4,4,0};case '>':return {16,8,4,2,4,8,16};
        default:return {};
    }
}
void quad(std::vector<Vertex>& out,float x,float y,float w,float h,DWORD color) {
    const Vertex a{x,y,0,1,color},b{x+w,y,0,1,color},c{x+w,y+h,0,1,color},d{x,y+h,0,1,color};
    out.insert(out.end(),{a,b,c,a,c,d});
}
void triangle(std::vector<Vertex>& out,float ax,float ay,float bx,float by,
              float cx,float cy,DWORD color) {
    out.insert(out.end(),{{ax,ay,0,1,color},{bx,by,0,1,color},{cx,cy,0,1,color}});
}
void smallText(std::vector<Vertex>& out,const char* text,float x,float y,
               float scale,DWORD color) {
    for(unsigned i=0;text[i]&&i<24;++i){
        const auto rows=glyph(text[i]);
        for(unsigned row=0;row<7;++row)for(unsigned col=0;col<5;++col)
            if(rows[row]&(1u<<(4-col)))
                quad(out,x+(6*i+col)*scale,y+row*scale,scale,scale,color);
    }
}
void beacon(std::vector<Vertex>& out,float cx,float cy,float right,float forward,
            float scale,DWORD color){
    if(!std::isfinite(right)||!std::isfinite(forward))return;
    const auto size=std::hypot(right,forward);
    if(size<0.5f||size>1.5f)return;
    const float dx=right/size,dy=-forward/size;
    // Correctly normalized player-relative vector, with arrow point outward.
    const float tipX=cx+dx*31*scale,tipY=cy+dy*31*scale;
    const float baseX=cx+dx*17*scale,baseY=cy+dy*17*scale;
    triangle(out,tipX,tipY,baseX-dy*6*scale,baseY+dx*6*scale,
        baseX+dy*6*scale,baseY-dx*6*scale,color);
    quad(out,cx-2*scale,cy-2*scale,4*scale,4*scale,D3DCOLOR_ARGB(255,240,240,240));
}
}
void configureNativeRivalHud(bool value) {enabled=value;}
void publishNativeRivalHud(const NativeRivalHudView& value) {
    std::lock_guard lock(mutex);current=value;publishedAt=GetTickCount64();
}
void renderNativeRivalHud(void* raw) try {
    if(!enabled.load()||!raw) return;
    NativeRivalHudView view{};std::uint64_t timestamp=0;
    {std::unique_lock lock(mutex,std::try_to_lock);if(!lock.owns_lock()) return;view=current;timestamp=publishedAt;}
    if(!view.visible||GetTickCount64()-timestamp>2000) return;
    auto* device=static_cast<IDirect3DDevice9*>(raw);
    D3DVIEWPORT9 viewport{};
    if(FAILED(device->GetViewport(&viewport))||viewport.Width<360||viewport.Height<200) return;
    const float scale=std::clamp(viewport.Width/1280.0f,1.0f,2.0f);
    const float x=static_cast<float>(viewport.X)+12,y=static_cast<float>(viewport.Y)+12;
    // CPU-only storage is reused on the rendering thread; no GPU resource
    // survives this callback or needs reset handling.
    thread_local std::vector<Vertex> vertices;vertices.clear();vertices.reserve(50000);
    quad(vertices,x,y,330*scale,85*scale,D3DCOLOR_ARGB(205,12,16,20));
    for(unsigned line=0;line<6;++line) for(unsigned i=0;i<52&&view.lines[line][i];++i) {
        const auto rows=glyph(view.lines[line][i]);
        for(unsigned row=0;row<7;++row) for(unsigned col=0;col<5;++col) if(rows[row]&(1u<<(4-col)))
            quad(vertices,x+(8+6*i+col)*scale,y+(8+11*line+row)*scale,scale,scale,
                line==0?D3DCOLOR_ARGB(255,100,220,255):
                    (line==2&&view.challengeReady?D3DCOLOR_ARGB(255,105,240,145):D3DCOLOR_ARGB(255,238,240,245)));
    }
    quad(vertices,x+8*scale,y+76*scale,312*scale,3*scale,D3DCOLOR_ARGB(255,50,55,62));
    const float progress=std::isfinite(view.holdProgress)?std::clamp(view.holdProgress,0.0f,1.0f):0;
    quad(vertices,x+8*scale,y+76*scale,312*scale*progress,3*scale,
        view.playerLeading?D3DCOLOR_ARGB(255,70,220,110):D3DCOLOR_ARGB(255,245,95,80));
    if(view.rivalBeaconValid) {
        // Separate FRR compass near the stock minimap: NOT a pin written
        // to the actual vanilla minimap/radar widget or map coordinates.
        const float cx=static_cast<float>(viewport.X)+55*scale;
        const float cy=static_cast<float>(viewport.Y+viewport.Height)-155*scale;
        quad(vertices,cx-41*scale,cy-46*scale,82*scale,91*scale,
            D3DCOLOR_ARGB(195,12,21,17));
        smallText(vertices,"RICO",cx-12*scale,cy-39*scale,scale,
            D3DCOLOR_ARGB(255,110,245,137));
        beacon(vertices,cx,cy,view.rivalRight,view.rivalFront,scale,
            D3DCOLOR_ARGB(255,60,237,118));
        if(view.rivalStationary)
            smallText(vertices,"PARADO",cx-19*scale,cy+36*scale,scale,
                D3DCOLOR_ARGB(255,255,175,72));
    }
    if(view.routeCueValid) {
        // Yellow arrow shows the ACTUAL OBSERVED leader trail in the
        // current Outrun; no invented intersections or race checkpoints.
        const float cx=static_cast<float>(viewport.X+viewport.Width/2);
        const float cy=static_cast<float>(viewport.Y)+150*scale;
        quad(vertices,cx-55*scale,cy-47*scale,110*scale,92*scale,
            D3DCOLOR_ARGB(190,20,20,14));
        smallText(vertices,"TRILHA RICO",cx-32*scale,cy-40*scale,scale,
            D3DCOLOR_ARGB(255,250,205,88));
        beacon(vertices,cx,cy,view.routeRight,view.routeFront,scale,
            D3DCOLOR_ARGB(255,255,191,55));
    }
    IDirect3DStateBlock9* state=nullptr;
    if(FAILED(device->CreateStateBlock(D3DSBT_ALL,&state))) return;
    if(FAILED(state->Capture())) {state->Release();return;}
    device->SetVertexShader(nullptr);device->SetPixelShader(nullptr);
    device->SetTexture(0,nullptr);device->SetTexture(1,nullptr);
    device->SetFVF(D3DFVF_XYZRHW|D3DFVF_DIFFUSE);
    device->SetRenderState(D3DRS_FILLMODE,D3DFILL_SOLID);
    device->SetRenderState(D3DRS_STENCILENABLE,FALSE);
    device->SetRenderState(D3DRS_ZENABLE,FALSE);device->SetRenderState(D3DRS_ZWRITEENABLE,FALSE);
    device->SetRenderState(D3DRS_LIGHTING,FALSE);device->SetRenderState(D3DRS_FOGENABLE,FALSE);
    device->SetRenderState(D3DRS_CULLMODE,D3DCULL_NONE);device->SetRenderState(D3DRS_SCISSORTESTENABLE,FALSE);
    device->SetRenderState(D3DRS_ALPHATESTENABLE,FALSE);device->SetRenderState(D3DRS_ALPHABLENDENABLE,TRUE);
    device->SetRenderState(D3DRS_SRCBLEND,D3DBLEND_SRCALPHA);device->SetRenderState(D3DRS_DESTBLEND,D3DBLEND_INVSRCALPHA);
    device->SetRenderState(D3DRS_BLENDOP,D3DBLENDOP_ADD);device->SetRenderState(D3DRS_SEPARATEALPHABLENDENABLE,FALSE);
    device->SetRenderState(D3DRS_COLORWRITEENABLE,0xf);device->SetRenderState(D3DRS_SRGBWRITEENABLE,FALSE);
    device->SetTextureStageState(0,D3DTSS_COLOROP,D3DTOP_SELECTARG1);
    device->SetTextureStageState(0,D3DTSS_COLORARG1,D3DTA_DIFFUSE);
    device->SetTextureStageState(0,D3DTSS_ALPHAOP,D3DTOP_SELECTARG1);
    device->SetTextureStageState(0,D3DTSS_ALPHAARG1,D3DTA_DIFFUSE);
    device->SetTextureStageState(1,D3DTSS_COLOROP,D3DTOP_DISABLE);
    device->DrawPrimitiveUP(D3DPT_TRIANGLELIST,static_cast<UINT>(vertices.size()/3),vertices.data(),sizeof(Vertex));
    state->Apply();state->Release();
} catch (...) { /* Optional HUD cannot propagate C++ allocation/lock errors into the game. */ }
}
