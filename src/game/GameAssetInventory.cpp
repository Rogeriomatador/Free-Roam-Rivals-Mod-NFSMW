#include "GameAssetInventory.h"
#include <algorithm>
#include <cctype>
#include <system_error>

namespace frr::game {
namespace {
bool ordinaryFile(const std::filesystem::path& path) {
    std::error_code error;
    const auto regular=std::filesystem::is_regular_file(path,error);
    if(error||!regular) return false;
    const auto size=std::filesystem::file_size(path,error);
    // A zero-byte placeholder does not prove a usable asset.
    return !error && size>0;
}
std::string uppercase(std::string_view key) {
    std::string out;
    out.reserve(key.size());
    for(unsigned char ch:key) out.push_back(static_cast<char>(std::toupper(ch)));
    return out;
}
} // namespace

bool validCarFolderKey(std::string_view key) {
    if(key.empty()||key.size()>48) return false;
    for(unsigned char c:key)
        if(!(c>='a'&&c<='z')&&!(c>='0'&&c<='9')&&c!='_') return false;
    return true;
}
GameAssetInventory inspectGameAssets(const std::filesystem::path& gameRoot,
    const std::vector<std::string_view>& keys) {
    GameAssetInventory result{};
    std::error_code error;
    result.rootKnown=std::filesystem::is_directory(gameRoot,error)&&!error;
    if(!result.rootKnown) return result;
    // Common retail layout. Addon tools may repack files; "missing" here means
    // "not found at the expected retail path", never "vehicle cannot spawn".
    result.globalA=ordinaryFile(gameRoot/L"GLOBAL"/L"GLOBALA.BUN");
    result.globalB=ordinaryFile(gameRoot/L"GLOBAL"/L"GLOBALB.LZC");
    result.track=ordinaryFile(gameRoot/L"TRACKS"/L"L2RA.BUN");
    result.streamTrack=ordinaryFile(gameRoot/L"TRACKS"/L"STREAML2RA.BUN");
    result.models.reserve(std::min<std::size_t>(keys.size(),128));
    for(std::size_t i=0;i<keys.size()&&i<128;++i) {
        GameAssetModel model{};
        model.key=std::string(keys[i]);
        model.safeKey=validCarFolderKey(keys[i]);
        if(model.safeKey) {
            const auto folder=gameRoot/L"CARS"/uppercase(keys[i]);
            model.geometry=ordinaryFile(folder/L"GEOMETRY.BIN");
            model.textures=ordinaryFile(folder/L"TEXTURES.BIN");
        }
        if(model.completePair()) ++result.completeModels;
        result.models.push_back(std::move(model));
    }
    return result;
}
} // namespace frr::game
