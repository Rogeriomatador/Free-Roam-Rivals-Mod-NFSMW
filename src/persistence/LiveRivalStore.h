#pragma once
#include "../domain/OutrunRace.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <optional>
#include <utility>

namespace frr::persistence {
struct LiveRivalRecord {
    std::uint64_t profile=0,battles=0,wins=0,losses=0,draws=0,aborts=0;
};
struct LiveRivalLoad {bool ok=false,found=false;LiveRivalRecord record{};std::string error;};
bool recordLiveResult(LiveRivalRecord& record,domain::OutrunOutcome result);
std::string serializeLiveRival(const LiveRivalRecord& record);
std::optional<LiveRivalRecord> parseLiveRival(std::string_view input);
class LiveRivalStore {
public:
    explicit LiveRivalStore(std::filesystem::path directory):directory_(std::move(directory)) {}
    std::filesystem::path pathFor(std::uint64_t profile) const;
    LiveRivalLoad load(std::uint64_t profile) const;
    bool save(const LiveRivalRecord& record,std::string& error) const;
private:
    std::filesystem::path directory_;
};
}
