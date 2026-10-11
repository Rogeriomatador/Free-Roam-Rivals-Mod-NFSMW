#pragma once
#include "../domain/WorldMetricCalibration.h"
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace frr::persistence {
// The cache is NOT a calibration source by itself. It only records a scale
// previously promoted from >=20 verified, consistent physical-motion samples.
std::string encodeWorldMetricCache(const domain::WorldMetricCalibration& metric,
    std::string_view executableMd5,unsigned verifiedSamples);
std::optional<domain::WorldMetricCalibration> decodeWorldMetricCache(
    std::string_view bytes,std::string_view executableMd5);

struct MetricCacheLoad {
    bool found=false,ok=false;
    domain::WorldMetricCalibration metric{};
    std::string error;
};
class WorldMetricCache {
public:
    explicit WorldMetricCache(std::filesystem::path path):path_(std::move(path)){}
    MetricCacheLoad load(std::string_view executableMd5) const;
    bool save(const domain::WorldMetricCalibration& metric,std::string_view executableMd5,
        unsigned verifiedSamples,std::string& error) const;
    bool discard(std::string& error) const;
private:
    std::filesystem::path path_;
};
} // namespace frr::persistence
