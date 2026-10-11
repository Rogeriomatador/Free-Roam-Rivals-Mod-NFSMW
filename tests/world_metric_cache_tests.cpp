#include "persistence/WorldMetricCache.h"
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>

#ifdef _WIN32
#include <windows.h>
#endif
using namespace frr;
static void require(bool ok,const char* reason) {
    if(!ok) {std::cerr<<reason<<'\n';std::exit(1);}
}
int main() {
    constexpr auto exe="C0516B485065FABDD69579816B5DF763";
    constexpr auto different="A0516B485065FABDD69579816B5DF763";
    domain::WorldMetricCalibration good{true,1.25f};
    const auto encoded=persistence::encodeWorldMetricCache(good,exe,20);
    require(!encoded.empty()&&encoded.size()<512,"verified metric encoded");
    const auto round=persistence::decodeWorldMetricCache(encoded,exe);
    require(round.has_value()&&round->verified&&
        std::bit_cast<std::uint32_t>(round->worldUnitsPerMeter)==std::bit_cast<std::uint32_t>(good.worldUnitsPerMeter),
        "cache preserves exact float bits");
    require(!persistence::decodeWorldMetricCache(encoded,different),"wrong executable cannot reuse scale");
    require(persistence::encodeWorldMetricCache(good,exe,19).empty(),"insufficient proof samples rejected");
    require(persistence::encodeWorldMetricCache(good,exe,121).empty(),"oversized proof sample count rejected");
    require(persistence::encodeWorldMetricCache({false,1.25f},exe,20).empty(),"unverified scale rejected");
    require(persistence::encodeWorldMetricCache({true,0.01f},exe,20).empty(),"tiny scale rejected");
    require(persistence::encodeWorldMetricCache({true,12.0f},exe,20).empty(),"large scale rejected");
    require(persistence::encodeWorldMetricCache(
        {true,std::numeric_limits<float>::quiet_NaN()},exe,20).empty(),"NaN rejected");
    auto damaged=encoded;damaged[damaged.find("SCALE_BITS=")+12]='A';
    require(!persistence::decodeWorldMetricCache(damaged,exe),"corrupt scale rejected");
    require(!persistence::decodeWorldMetricCache(encoded+"EXTRA=1\n",exe),"trailing input rejected");
    require(!persistence::decodeWorldMetricCache(std::string(600,'0'),exe),"oversized file rejected");

#ifdef _WIN32
    auto dir=std::filesystem::temp_directory_path()/
        ("frr-world-metric-tests-"+std::to_string(GetCurrentProcessId()));
#else
    auto dir=std::filesystem::temp_directory_path()/"frr-world-metric-tests";
#endif
    std::error_code ec;
    std::filesystem::remove_all(dir,ec);
    persistence::WorldMetricCache storage(dir/"world-metric-v1.state");
    require(!storage.load(exe).found,"missing cache never fabricates calibration");
    std::string error;
    require(storage.save(good,exe,20,error),"atomic file write");
    auto loaded=storage.load(exe);
    require(loaded.found&&loaded.ok&&std::abs(loaded.metric.worldUnitsPerMeter-1.25f)<0.00001f,
        "second startup can load metric without motion");
    require(!storage.load(different).ok,"storage bound to executable fingerprint");
    require(storage.discard(error),"cache can be safely invalidated after contradictory evidence");
    require(!storage.load(exe).found,"invalidated cache no longer available");
    require(storage.save(good,exe,20,error),"save after invalidation");
    {std::ofstream corrupt(dir/"world-metric-v1.state",std::ios::binary|std::ios::trunc);corrupt<<"bad"; }
    const auto unreadable=storage.load(exe);
    require(unreadable.found&&!unreadable.ok,"damaged cache rejected, never silently trusted");
    require(storage.save(good,exe,25,error),"verified observation safely replaces damaged cache");
    require(storage.load(exe).ok,"recovered verified cache");
    std::filesystem::remove_all(dir,ec);
    std::cout<<"World metric fingerprint, stable-proof, integrity, atomic persistence and rejection tests passed\n";
}
