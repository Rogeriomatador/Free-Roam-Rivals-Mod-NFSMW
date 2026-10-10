#pragma once
#include "../domain/WorldMetricCalibration.h"
namespace frr::game {
// Explicit opt-in. Configure before hook installation; tick only after the
// original gameplay update. F8 requests one creation; optional configured hold requests guarded retirement.
void configureNativeRivalPrototype(bool enabled, bool nearPlayer, unsigned retireKey=0, bool requireModifiers=true, float holdSeconds=1.5f);
const char* nativeRivalRetirementHint();
void tickNativeRivalPrototype(const domain::WorldMetricCalibration& metric, float updateDelta);
}
