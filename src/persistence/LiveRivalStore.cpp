#include "LiveRivalStore.h"
#include <array>
#include <charconv>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <system_error>
#ifdef _WIN32
#include <windows.h>
#endif

namespace frr::persistence {
namespace {
bool valid(const LiveRivalRecord& r) {
    return r.profile&&r.battles<=1000000&&r.wins<=r.battles&&r.losses<=r.battles&&
        r.draws<=r.battles&&r.aborts<=r.battles&&r.wins+r.losses+r.draws+r.aborts==r.battles;
}
}
bool recordLiveResult(LiveRivalRecord& r,domain::OutrunOutcome result) {
    if(!valid(r)||r.battles==1000000||result==domain::OutrunOutcome::InProgress) return false;
    switch(result) {
        case domain::OutrunOutcome::PlayerWon:++r.wins;break;
        case domain::OutrunOutcome::RivalWon:++r.losses;break;
        case domain::OutrunOutcome::Draw:++r.draws;break;
        case domain::OutrunOutcome::Aborted:++r.aborts;break;
        default:return false;
    }
    ++r.battles;return true;
}
std::string serializeLiveRival(const LiveRivalRecord& r) {
    if(!valid(r)) return {};
    std::ostringstream s;
    s<<"FRR_LIVE_V1\nprofile="<<r.profile<<"\nbattles="<<r.battles<<"\nwins="<<r.wins
        <<"\nlosses="<<r.losses<<"\ndraws="<<r.draws<<"\naborts="<<r.aborts<<'\n';
    return s.str();
}
std::optional<LiveRivalRecord> parseLiveRival(std::string_view text) {
    if(text.size()>512||!text.starts_with("FRR_LIVE_V1\n")) return {};
    text.remove_prefix(12);
    constexpr std::array<std::string_view,6> keys{"profile=","battles=","wins=","losses=","draws=","aborts="};
    std::array<std::uint64_t,6> values{};
    for(std::size_t i=0;i<keys.size();++i) {
        if(!text.starts_with(keys[i])) return {};
        text.remove_prefix(keys[i].size());
        const auto newline=text.find('\n');
        if(newline==std::string_view::npos||newline==0) return {};
        const auto number=text.substr(0,newline);
        const auto parsed=std::from_chars(number.data(),number.data()+number.size(),values[i]);
        if(parsed.ec!=std::errc{}||parsed.ptr!=number.data()+number.size()) return {};
        text.remove_prefix(newline+1);
    }
    if(!text.empty()) return {};
    const LiveRivalRecord r{values[0],values[1],values[2],values[3],values[4],values[5]};
    return valid(r)?std::optional<LiveRivalRecord>(r):std::nullopt;
}
std::filesystem::path LiveRivalStore::pathFor(std::uint64_t profile) const {
    std::ostringstream name;name<<"live-rico-"<<std::hex<<std::setfill('0')<<std::setw(16)<<profile<<".state";
    return directory_/name.str();
}
LiveRivalLoad LiveRivalStore::load(std::uint64_t profile) const {
    LiveRivalLoad out{};out.record.profile=profile;
    if(!profile) {out.error="Missing profile identity";return out;}
    std::error_code error;
    const auto path=pathFor(profile);
    const bool exists=std::filesystem::exists(path,error);
    if(error) {out.error=error.message();return out;}
    if(!exists) {out.ok=true;return out;}
    out.found=true;
    const auto size=std::filesystem::file_size(path,error);
    if(error||size>512) {out.error="Invalid or oversized rival state; original preserved";return out;}
    std::ifstream stream(path,std::ios::binary);
    std::string text(static_cast<std::size_t>(size),'\0');
    stream.read(text.data(),static_cast<std::streamsize>(text.size()));
    const auto parsed=parseLiveRival(text);
    if(!stream||!parsed||parsed->profile!=profile) {out.error="Invalid rival state or profile mismatch; original preserved";return out;}
    out.ok=true;out.record=*parsed;return out;
}
bool LiveRivalStore::save(const LiveRivalRecord& record,std::string& error) const {
    const auto text=serializeLiveRival(record);
    if(text.empty()) {error="Invalid rival record";return false;}
    // Never replace corrupt, mismatched or unparseable existing data.
    const auto current=load(record.profile);
    if(!current.ok) {error=current.error;return false;}
    if(current.record.battles>record.battles) {error="Stale rival record; newer history preserved";return false;}
    if(current.found&&current.record.battles==record.battles&&serializeLiveRival(current.record)!=text) {
        error="Conflicting rival record; existing history preserved";return false;
    }
    std::error_code ec;std::filesystem::create_directories(directory_,ec);
    if(ec) {error=ec.message();return false;}
    const auto target=pathFor(record.profile);
    auto temp=target;temp+=".tmp";
#ifdef _WIN32
    HANDLE file=CreateFileW(temp.c_str(),GENERIC_WRITE,0,nullptr,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,nullptr);
    if(file==INVALID_HANDLE_VALUE) {error="Could not create rival state temporary file";return false;}
    DWORD written=0;
    const bool wrote=WriteFile(file,text.data(),static_cast<DWORD>(text.size()),&written,nullptr)!=FALSE&&
        written==text.size()&&FlushFileBuffers(file)!=FALSE;
    CloseHandle(file);
    const bool replaced=wrote&&MoveFileExW(temp.c_str(),target.c_str(),MOVEFILE_REPLACE_EXISTING|MOVEFILE_WRITE_THROUGH)!=FALSE;
#else
    std::ofstream output(temp,std::ios::binary|std::ios::trunc);
    output.write(text.data(),static_cast<std::streamsize>(text.size()));output.flush();
    const bool wrote=output.good();output.close();
    if(wrote) std::filesystem::rename(temp,target,ec);
    const bool replaced=wrote&&!ec;
#endif
    if(!replaced) {std::filesystem::remove(temp,ec);error="Could not atomically save rival state";return false;}
    return true;
}
}
