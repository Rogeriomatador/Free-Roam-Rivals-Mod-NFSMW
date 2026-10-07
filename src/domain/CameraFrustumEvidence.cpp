#include "CameraFrustumEvidence.h"
#include <algorithm>
#include <array>
#include <cmath>

namespace frr::domain {
namespace {
bool finiteMatrix(const CameraMatrix4& value) {
    for (const auto& row : value.m) for (float v : row) if (!std::isfinite(v)) return false;
    return true;
}
bool close(float a, float b, float tolerance = 0.002f) {
    return std::abs(a - b) <= tolerance * std::max({1.0f, std::abs(a), std::abs(b)});
}
float dot(SpatialVector3 a, SpatialVector3 b) { return a.x*b.x + a.y*b.y + a.z*b.z; }
SpatialVector3 unit(SpatialVector3 a) {
    const float length = std::sqrt(dot(a,a));
    return {a.x/length, a.y/length, a.z/length};
}
}
// MW05 eSwizzleWorldVector -> bConvertFromBond: (z, -x, y).
SpatialVector3 simulationToRender(const SpatialVector3& p) { return {p.z, -p.x, p.y}; }

bool coherentPrimaryCamera(const PrimaryCameraSample& s) {
    if (!s.available || !s.active || !s.coordinateMappingEstablished || s.viewId != 1 ||
        !finiteMatrix(s.view) || !finiteMatrix(s.projection) || !finiteMatrix(s.viewProjection) ||
        !std::isfinite(s.eyeRender.x) || !std::isfinite(s.eyeRender.y) || !std::isfinite(s.eyeRender.z)) return false;
    const auto& v = s.view.m;
    const auto& p = s.projection.m;
    // Rigid row-vector view and D3D perspective shape. No zero/identity,
    // transposed or stale matrices can become off-screen proof.
    if (!close(v[0][3],0) || !close(v[1][3],0) || !close(v[2][3],0) || !close(v[3][3],1) ||
        !close(p[3][3],0) || !close(std::abs(p[2][3]),1) ||
        std::abs(p[0][0]) < 1e-5f || std::abs(p[1][1]) < 1e-5f || std::abs(p[3][2]) < 1e-5f ||
        !close(p[0][3],0) || !close(p[1][3],0) || !close(p[3][0],0) || !close(p[3][1],0)) return false;
    for (int a=0; a<3; ++a) for (int b=0; b<3; ++b) {
        float value=0;
        for (int k=0; k<3; ++k) value += v[a][k]*v[b][k];
        if (!close(value, a==b ? 1.0f : 0.0f)) return false;
    }
    const float eye[4]{s.eyeRender.x,s.eyeRender.y,s.eyeRender.z,1};
    for (int j=0; j<3; ++j) {
        double value=0;
        for (int k=0; k<4; ++k) value += static_cast<double>(eye[k])*v[k][j];
        if (std::abs(value) > 0.1) return false;
    }
    for (int i=0; i<4; ++i) for (int j=0; j<4; ++j) {
        double product=0;
        for (int k=0; k<4; ++k) product += static_cast<double>(v[i][k])*p[k][j];
        if (!std::isfinite(product) || !close(static_cast<float>(product),s.viewProjection.m[i][j])) return false;
    }
    return true;
}

CameraFrustumReport classifyPrimaryCameraFootprint(
    const PrimaryCameraSample& camera, const VehicleOrientedBox& box
) {
    CameraFrustumReport out{};
    if (!coherentPrimaryCamera(camera) || !validVehicleOrientedBox(box)) return out;
    const auto center=simulationToRender(box.center);
    const std::array<SpatialVector3,3> axis{
        simulationToRender(unit(box.right)),simulationToRender(unit(box.up)),simulationToRender(unit(box.forward))};
    const float extent[3]{box.halfExtents.x,box.halfExtents.y,box.halfExtents.z};
    // Row-vector clip coordinates: -w <= x/y <= w, 0 <= z <= w.
    // Extract inward planes from columns, then test the OBB's entire support
    // interval. A center outside a plane is insufficient by itself.
    std::array<std::array<double,4>,6> planes{};
    const auto& m=camera.viewProjection.m;
    for (int row=0; row<4; ++row) {
        planes[0][row]=static_cast<double>(m[row][3])+m[row][0];
        planes[1][row]=static_cast<double>(m[row][3])-m[row][0];
        planes[2][row]=static_cast<double>(m[row][3])+m[row][1];
        planes[3][row]=static_cast<double>(m[row][3])-m[row][1];
        planes[4][row]=m[row][2];
        planes[5][row]=static_cast<double>(m[row][3])-m[row][2];
    }
    // Validate every plane before classifying any one plane.
    for (auto& plane:planes) {
        const double norm=std::sqrt(plane[0]*plane[0]+plane[1]*plane[1]+plane[2]*plane[2]);
        if (!std::isfinite(norm) || norm<1e-9) return out;
        for (double& component:plane) component/=norm;
    }
    out.queryVerified=true;
    out.visibility=CameraBoxVisibility::PotentiallyVisible;
    for (int i=0; i<6; ++i) {
        const auto& q=planes[i];
        const double distance=q[0]*center.x+q[1]*center.y+q[2]*center.z+q[3];
        double radius=0;
        for (int k=0; k<3; ++k) radius+=extent[k]*std::abs(q[0]*axis[k].x+q[1]*axis[k].y+q[2]*axis[k].z);
        if (!std::isfinite(distance) || !std::isfinite(radius)) return {};
        if (distance+radius < -0.01) {
            out.visibility=CameraBoxVisibility::OutsideFrustum;
            out.separatingPlane=i;
            break;
        }
    }
    return out;
}
const char* cameraBoxVisibilityName(CameraBoxVisibility state) {
    switch(state) {
        case CameraBoxVisibility::Unverified:return "unverified";
        case CameraBoxVisibility::OutsideFrustum:return "outside_primary_frustum";
        case CameraBoxVisibility::PotentiallyVisible:return "potentially_visible";
    }
    return "unverified";
}
} // namespace frr::domain
