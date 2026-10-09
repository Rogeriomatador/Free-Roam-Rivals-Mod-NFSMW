#pragma once
#include "../domain/WorldMetricCalibration.h"
namespace frr::game {
void logDiagnosticModuleInventory(const char* reason);
void tickDiagnosticBundle(const domain::WorldMetricCalibration& metric, float updateDelta);
}
