#include "domain/CameraFrustumEvidence.h"
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>

namespace {
void require(bool ok,const char* message) {
    if(!ok) { std::cerr << "FAILED: " << message << '\n'; std::exit(1); }
}
frr::domain::PrimaryCameraSample camera() {
    using namespace frr::domain;
    PrimaryCameraSample c{};
    c.available=c.active=c.coordinateMappingEstablished=true;c.viewId=1;
    for(int i=0;i<4;++i)c.view.m[i][i]=1;
    // D3D row-vector LH camera, +render Z, near=1/far=100, 90 deg FOV.
    c.projection.m[0][0]=c.projection.m[1][1]=1;
    c.projection.m[2][2]=100.0f/99.0f;
    c.projection.m[2][3]=1;
    c.projection.m[3][2]=-100.0f/99.0f;
    c.viewProjection=c.projection;
    return c;
}
frr::domain::VehicleOrientedBox boxAtRender(float x,float y,float z) {
    frr::domain::VehicleOrientedBox b{};
    b.valid=true;b.center={-y,z,x}; // inverse swizzle
    b.right={1,0,0};b.up={0,1,0};b.forward={0,0,1};b.halfExtents={1,1,1};
    return b;
}
}
int main() {
    using namespace frr::domain;
    const auto converted=simulationToRender({2,3,4});
    require(converted.x==4 && converted.y==-2 && converted.z==3,"engine swizzle is z,-x,y");
    auto c=camera();
    require(coherentPrimaryCamera(c),"coherent D3D camera validated");
    auto b=boxAtRender(0,0,10);
    auto r=classifyPrimaryCameraFootprint(c,b);
    require(r.queryVerified && r.visibility==CameraBoxVisibility::PotentiallyVisible,"front box potentially visible");
    for(const auto pos : {SpatialVector3{-30,0,10},SpatialVector3{30,0,10},SpatialVector3{0,-30,10},
                          SpatialVector3{0,30,10},SpatialVector3{0,0,-10},SpatialVector3{0,0,110}}) {
        r=classifyPrimaryCameraFootprint(c,boxAtRender(pos.x,pos.y,pos.z));
        require(r.queryVerified && r.visibility==CameraBoxVisibility::OutsideFrustum,"box wholly outside each plane");
        require(!r.spawnVisibilityVerified,"primary frustum never implies complete spawn visibility");
    }
    r=classifyPrimaryCameraFootprint(c,boxAtRender(10.5f,0,10));
    require(r.visibility==CameraBoxVisibility::PotentiallyVisible,"center outside still visible when box intersects");
    r=classifyPrimaryCameraFootprint(c,boxAtRender(12,0,10));
    require(r.visibility==CameraBoxVisibility::PotentiallyVisible,"plane contact remains potentially visible");
    r=classifyPrimaryCameraFootprint(c,boxAtRender(12.1f,0,10));
    require(r.visibility==CameraBoxVisibility::OutsideFrustum,"whole support interval separates");
    b=boxAtRender(12,0,10);b.right={0.70710678f,0,-0.70710678f};b.forward={0.70710678f,0,0.70710678f};
    b.halfExtents.z=3;
    require(classifyPrimaryCameraFootprint(c,b).visibility==CameraBoxVisibility::PotentiallyVisible,"rotated long OBB remains visible");
    for(int i=0;i<8;++i) {
        auto invalid=c;
        switch(i) {
            case 0:invalid.active=false;break;
            case 1:invalid.available=false;break;
            case 2:invalid.coordinateMappingEstablished=false;break;
            case 3:invalid.viewId=3;break;
            case 4:invalid.viewProjection.m[0][0]+=1;break;
            case 5:invalid.eyeRender.x=1;break;
            case 6:invalid.projection.m[0][0]=std::numeric_limits<float>::quiet_NaN();break;
            case 7:invalid.view={};break;
        }
        require(!classifyPrimaryCameraFootprint(invalid,boxAtRender(30,0,10)).queryVerified,"invalid camera fails closed");
    }
    auto translated=c;
    translated.eyeRender={200,-80,30};
    translated.view.m[3][0]=-200;translated.view.m[3][1]=80;translated.view.m[3][2]=-30;
    for(int i=0;i<4;++i)for(int j=0;j<4;++j) {
        translated.viewProjection.m[i][j]=0;
        for(int k=0;k<4;++k)translated.viewProjection.m[i][j]+=translated.view.m[i][k]*translated.projection.m[k][j];
    }
    require(classifyPrimaryCameraFootprint(translated,boxAtRender(200,-80,40)).visibility==CameraBoxVisibility::PotentiallyVisible,
            "translated world camera uses correct matrix order");
    b=boxAtRender(0,0,10);b.forward={0,0,0};
    require(!classifyPrimaryCameraFootprint(c,b).queryVerified,"invalid footprint fails closed");
    std::cout << "Free Roam Rivals camera tests passed.\n";
}
