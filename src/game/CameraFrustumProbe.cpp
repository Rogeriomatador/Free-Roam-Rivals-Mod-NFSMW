#include "CameraFrustumProbe.h"
#include <mwsdk/game/mw05_camera.hpp>
#include <cstdint>

namespace frr::game {
namespace {
frr::domain::CameraMatrix4 copyMatrix(const mwsdk::mw05::Mat4& source) {
    frr::domain::CameraMatrix4 target{};
    for (int i=0;i<4;++i) for(int j=0;j<4;++j) target.m[i][j]=source.m[i][j];
    return target;
}
}
frr::domain::PrimaryCameraSample CameraFrustumProbe::sample() {
    namespace cam=mwsdk::mw05::camera;
    frr::domain::PrimaryCameraSample out{};
    // The pinned SDK supplies exact PC addresses and guarded copies. No camera
    // ownership changes, engine calls, hook additions or guessed subobjects.
    const auto view=cam::player_view();
    const auto id=cam::detail::load<std::uint32_t>(cam::detail::at(view,cam::off::eView::Id));
    const auto active=cam::detail::load<std::uint8_t>(cam::detail::at(view,cam::off::eView::Active));
    const auto info=cam::detail::load_ptr(cam::detail::at(view,cam::off::eView::PlatInfo));
    const auto camera=cam::player_camera();
    if(!id || !active || !info || !camera || *id!=1 || *active!=1) return out;
    const auto eye=cam::detail::load<mwsdk::mw05::Vec3>(cam::detail::at(*camera,cam::off::Camera::Position));
    const auto v=cam::detail::load<mwsdk::mw05::Mat4>(cam::detail::at(*info,cam::off::ViewPlatInfo::View));
    const auto p=cam::detail::load<mwsdk::mw05::Mat4>(cam::detail::at(*info,cam::off::ViewPlatInfo::Projection));
    const auto vp=cam::detail::load<mwsdk::mw05::Mat4>(cam::detail::at(*info,cam::off::ViewPlatInfo::ViewProjection));
    const auto infoAfter=cam::detail::load_ptr(cam::detail::at(view,cam::off::eView::PlatInfo));
    const auto cameraAfter=cam::player_camera();
    if(!eye || !v || !p || !vp || !infoAfter || !cameraAfter || *info!=*infoAfter || *camera!=*cameraAfter) return out;
    out.available=true; out.active=true; out.viewId=*id;
    out.coordinateMappingEstablished=true;
    out.eyeRender={eye->x,eye->y,eye->z};
    out.view=copyMatrix(*v); out.projection=copyMatrix(*p); out.viewProjection=copyMatrix(*vp);
    if(!frr::domain::coherentPrimaryCamera(out)) return {};
    return out;
}
} // namespace frr::game
