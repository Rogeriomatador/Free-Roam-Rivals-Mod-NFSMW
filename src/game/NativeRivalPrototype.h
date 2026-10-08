#pragma once
#include "../domain/WorldMetricCalibration.h"
namespace frr::game {
// Explicit opt-in. Configure before hook installation; tick only after the
// original gameplay update. F8 requests one creation or safe retirement.
void configureNativeRivalPrototype(bool enabled, bool nearPlayer);
void tickNativeRivalPrototype(const domain::WorldMetricCalibration& metric, float updateDelta);
}
