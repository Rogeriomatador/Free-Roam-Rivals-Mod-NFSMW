#define CINTERFACE
#include <windows.h>
#include <d3d9.h>
#include "game/NativeRivalHud.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
namespace {
IDirect3DDevice9Vtbl methods{};
IDirect3DStateBlock9Vtbl blockMethods{};
IDirect3DDevice9 device{&methods};
IDirect3DStateBlock9 block{&blockMethods};
std::map<D3DRENDERSTATETYPE,DWORD> states, saved;
unsigned draws=0,captures=0,applies=0,releases=0,mutations=0;
bool failCreate=false,failCapture=false;
void require(bool ok,const char* message){if(!ok){std::cerr<<message<<'\n';std::exit(1);}}
HRESULT STDMETHODCALLTYPE viewport(IDirect3DDevice9*,D3DVIEWPORT9* out){*out={0,0,1280,720,0,1};return D3D_OK;}
HRESULT STDMETHODCALLTYPE create(IDirect3DDevice9*,D3DSTATEBLOCKTYPE type,IDirect3DStateBlock9** out){require(type==D3DSBT_ALL,"HUD captures all device states");if(failCreate)return E_OUTOFMEMORY;*out=&block;return D3D_OK;}
HRESULT STDMETHODCALLTYPE capture(IDirect3DStateBlock9*){++captures;if(failCapture)return D3DERR_INVALIDCALL;saved=states;return D3D_OK;}
HRESULT STDMETHODCALLTYPE apply(IDirect3DStateBlock9*){++applies;states=saved;return D3D_OK;}
ULONG STDMETHODCALLTYPE release(IDirect3DStateBlock9*){++releases;return 0;}
HRESULT STDMETHODCALLTYPE renderState(IDirect3DDevice9*,D3DRENDERSTATETYPE key,DWORD value){require(captures>applies,"state captured before HUD writes");++mutations;states[key]=value;return D3D_OK;}
HRESULT STDMETHODCALLTYPE texture(IDirect3DDevice9*,DWORD,IDirect3DBaseTexture9*){return D3D_OK;}
HRESULT STDMETHODCALLTYPE vertexShader(IDirect3DDevice9*,IDirect3DVertexShader9*){return D3D_OK;}
HRESULT STDMETHODCALLTYPE pixelShader(IDirect3DDevice9*,IDirect3DPixelShader9*){return D3D_OK;}
HRESULT STDMETHODCALLTYPE fvf(IDirect3DDevice9*,DWORD){return D3D_OK;}
HRESULT STDMETHODCALLTYPE stage(IDirect3DDevice9*,DWORD,D3DTEXTURESTAGESTATETYPE,DWORD){return D3D_OK;}
HRESULT STDMETHODCALLTYPE draw(IDirect3DDevice9*,D3DPRIMITIVETYPE type,UINT count,const void* vertices,UINT stride){require(type==D3DPT_TRIANGLELIST&&count>0&&vertices&&stride==20,"bounded valid HUD vertices");require(captures>applies,"draw inside captured state window");++draws;return D3D_OK;}
}
int main(){
    methods.GetViewport=viewport;methods.CreateStateBlock=create;methods.SetRenderState=renderState;
    methods.SetTexture=texture;methods.SetVertexShader=vertexShader;methods.SetPixelShader=pixelShader;
    methods.SetFVF=fvf;methods.SetTextureStageState=stage;methods.DrawPrimitiveUP=draw;
    blockMethods.Capture=capture;blockMethods.Apply=apply;blockMethods.Release=release;
    using namespace frr::game;
    NativeRivalHudView view{};view.visible=true;view.holdProgress=0.5f;std::strcpy(view.lines[0],"RICO / GOLF GTI");
    states[D3DRS_ZENABLE]=D3DZB_TRUE;states[D3DRS_ALPHABLENDENABLE]=FALSE;
    const auto original=states;publishNativeRivalHud(view);configureNativeRivalHud(true);
    renderNativeRivalHud(&device);
    require(draws==1&&captures==1&&applies==1&&releases==1,"draw then restore and release state block");
    require(states==original,"original render states restored");
    failCapture=true;const auto previousMutations=mutations;renderNativeRivalHud(&device);
    require(draws==1&&mutations==previousMutations&&releases==2,"failed capture cannot mutate device; block released");
    failCapture=false;failCreate=true;renderNativeRivalHud(&device);
    require(draws==1&&mutations==previousMutations,"failed state allocation cannot mutate device");
    configureNativeRivalHud(false);renderNativeRivalHud(&device);require(draws==1,"disabled HUD skips device");
    std::cout<<"HUD COM call ordering, restoration and allocation failures passed (mock device; not gameplay rendering)\n";
}
