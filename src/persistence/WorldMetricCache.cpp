#include "WorldMetricCache.h"

#include <bit>
#include <charconv>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#if defined(_WIN32)
#include <windows.h>
#endif

namespace frr::persistence {
namespace {
constexpr std::string_view kHeader="FRR_VERIFIED_WORLD_METRIC_V1";
constexpr unsigned kMinProof=20,kMaxProof=120;
constexpr std::uint64_t kFnvOffset=14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime=1099511628211ULL;
bool fingerprintValid(std::string_view md5) {
    if(md5.size()!=32) return false;
    for(const char ch:md5)
        if(!(ch>='0'&&ch<='9')&&!(ch>='A'&&ch<='F')) return false;
    return true;
}
bool scaleBounded(const domain::WorldMetricCalibration& metric) {
    return domain::validWorldMetricCalibration(metric)&&
        metric.worldUnitsPerMeter>=0.10f&&metric.worldUnitsPerMeter<=10.0f;
}
template<typename T>std::string hexValue(T number,unsigned width) {
    std::ostringstream o;
    o<<std::uppercase<<std::hex<<std::setfill('0')<<std::setw(width)<<number;
    return o.str();
}
std::uint64_t checksum(std::string_view bytes) {
    auto hash=kFnvOffset;
    for(unsigned char byte:bytes) hash=(hash^byte)*kFnvPrime;
    return hash;
}
template<typename T>bool parseUnsigned(std::string_view text,int base,T& result) {
    const char* first=text.data();
    auto parse=std::from_chars(first,first+text.size(),result,base);
    return parse.ec==std::errc{} && parse.ptr==first+text.size();
}
} // namespace

std::string encodeWorldMetricCache(const domain::WorldMetricCalibration& metric,
    std::string_view executableMd5,unsigned verifiedSamples) {
    if(!fingerprintValid(executableMd5)||!scaleBounded(metric)||
       verifiedSamples<kMinProof||verifiedSamples>kMaxProof) return {};
    std::string body=std::string(kHeader)+"\nEXE_MD5="+std::string(executableMd5)+
        "\nSCALE_BITS="+hexValue(std::bit_cast<std::uint32_t>(metric.worldUnitsPerMeter),8)+
        "\nPROOF_SAMPLES="+std::to_string(verifiedSamples)+"\n";
    return body+"CHECKSUM="+hexValue(checksum(body),16)+"\n";
}
std::optional<domain::WorldMetricCalibration> decodeWorldMetricCache(
    std::string_view bytes,std::string_view executableMd5) {
    if(bytes.empty()||bytes.size()>512||!fingerprintValid(executableMd5)) return std::nullopt;
    const std::string data(bytes);
    std::istringstream input(data);
    std::string header,exe,scale,proof,check,extra;
    if(!std::getline(input,header)||!std::getline(input,exe)||
       !std::getline(input,scale)||!std::getline(input,proof)||
       !std::getline(input,check)||std::getline(input,extra)) return std::nullopt;
    if(header!=kHeader || exe!="EXE_MD5="+std::string(executableMd5) ||
       !scale.starts_with("SCALE_BITS=")||!proof.starts_with("PROOF_SAMPLES=")||
       !check.starts_with("CHECKSUM=")) return std::nullopt;
    const auto bitsText=std::string_view(scale).substr(11);
    const auto proofText=std::string_view(proof).substr(14);
    const auto checkText=std::string_view(check).substr(9);
    if(bitsText.size()!=8||checkText.size()!=16) return std::nullopt;
    std::uint32_t bits=0;unsigned samples=0;std::uint64_t hash=0;
    if(!parseUnsigned(bitsText,16,bits)||!parseUnsigned(proofText,10,samples)||
       !parseUnsigned(checkText,16,hash)) return std::nullopt;
    const domain::WorldMetricCalibration metric{true,std::bit_cast<float>(bits)};
    const auto canonical=encodeWorldMetricCache(metric,executableMd5,samples);
    if(canonical.empty()||canonical!=data) return std::nullopt;
    // Full canonical match also validates checksum, proof, bounds and encoding.
    return metric;
}
MetricCacheLoad WorldMetricCache::load(std::string_view executableMd5) const {
    MetricCacheLoad out{};
    std::error_code ec;
    out.found=std::filesystem::exists(path_,ec);
    if(ec) {out.error=ec.message();return out;}
    if(!out.found) return out;
    const auto size=std::filesystem::file_size(path_,ec);
    if(ec||size==0||size>512) {out.error="empty or oversized world metric cache";return out;}
    std::ifstream input(path_,std::ios::binary);
    if(!input) {out.error="world metric cache cannot be opened";return out;}
    std::string bytes(static_cast<std::size_t>(size),'\0');
    input.read(bytes.data(),static_cast<std::streamsize>(size));
    if(!input) {out.error="world metric cache truncated";return out;}
    const auto decoded=decodeWorldMetricCache(bytes,executableMd5);
    if(!decoded) {out.error="world metric cache proof, checksum or executable mismatch";return out;}
    out.ok=true;out.metric=*decoded;return out;
}
bool WorldMetricCache::save(const domain::WorldMetricCalibration& metric,
    std::string_view executableMd5,unsigned verifiedSamples,std::string& error) const {
    const auto bytes=encodeWorldMetricCache(metric,executableMd5,verifiedSamples);
    if(bytes.empty()) {error="unverified/invalid world metric or executable";return false;}
    std::error_code ec;
    std::filesystem::create_directories(path_.parent_path(),ec);
    if(ec) {error=ec.message();return false;}
    auto temp=path_;temp+=".tmp";
#if defined(_WIN32)
    const auto file=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) {error="world metric cache temporary file cannot be created";return false;}
    DWORD written=0;
    const bool wrote=WriteFile(file,bytes.data(),static_cast<DWORD>(bytes.size()),&written,nullptr)!=FALSE &&
        written==bytes.size() && FlushFileBuffers(file)!=FALSE;
    CloseHandle(file);
    const bool replaced=wrote && MoveFileExW(temp.c_str(),path_.c_str(),
        MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
#else
    std::ofstream stream(temp,std::ios::binary|std::ios::trunc);
    stream.write(bytes.data(),static_cast<std::streamsize>(bytes.size()));
    stream.flush();
    const bool wrote=stream.good();stream.close();
    if(wrote) std::filesystem::rename(temp,path_,ec);
    const bool replaced=wrote&&!ec;
#endif
    if(!replaced) {
        std::filesystem::remove(temp,ec);
        error="world metric cache atomic replacement failed";return false;
    }
    return true;
}
bool WorldMetricCache::discard(std::string& error) const {
    std::error_code ec;
    std::filesystem::remove(path_,ec);
    if(ec) {error=ec.message();return false;}
    return true;
}
} // namespace frr::persistence
