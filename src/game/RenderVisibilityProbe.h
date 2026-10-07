#pragma once

#include "../domain/RenderVisibilityEvidence.h"

namespace frr::game {

class RenderVisibilityProbe {
public:
    static frr::domain::RenderFrustumSnapshot
    capture(void* rawD3D9Device);
};

} // namespace frr::game
