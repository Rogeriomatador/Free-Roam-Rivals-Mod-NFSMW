#include "RenderVisibilityProbe.h"

#include <d3d9.h>

#include <cmath>
#include <cstring>

namespace frr::game {
namespace {

frr::domain::RenderMatrix4 copyMatrix(
    const D3DMATRIX& source
) {
    frr::domain::RenderMatrix4 out{};

    static_assert(
        sizeof(out.m) == sizeof(source.m),
        "D3D matrix storage mismatch"
    );

    std::memcpy(
        out.m,
        source.m,
        sizeof(out.m)
    );

    return out;
}

} // namespace

frr::domain::RenderFrustumSnapshot
RenderVisibilityProbe::capture(
    void* rawD3D9Device
) {
    frr::domain::RenderFrustumSnapshot out{};

    if (!rawD3D9Device) {
        return out;
    }

    auto* device =
        static_cast<IDirect3DDevice9*>(
            rawD3D9Device
        );

    D3DVIEWPORT9 viewport{};
    D3DMATRIX view{};
    D3DMATRIX projection{};

    const HRESULT viewportResult =
        device->GetViewport(&viewport);
    const HRESULT viewResult =
        device->GetTransform(
            D3DTS_VIEW,
            &view
        );
    const HRESULT projectionResult =
        device->GetTransform(
            D3DTS_PROJECTION,
            &projection
        );

    if (FAILED(viewportResult) ||
        FAILED(viewResult) ||
        FAILED(projectionResult) ||
        viewport.Width == 0 ||
        viewport.Height == 0) {
        return out;
    }

    out.viewportWidth = viewport.Width;
    out.viewportHeight = viewport.Height;
    out.view = copyMatrix(view);
    out.projection = copyMatrix(projection);

    out.captureValid =
        frr::domain::finiteRenderMatrix(
            out.view
        ) &&
        frr::domain::finiteRenderMatrix(
            out.projection
        );

    return out;
}

} // namespace frr::game
