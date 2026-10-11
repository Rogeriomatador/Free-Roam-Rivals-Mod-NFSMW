#pragma once
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace frr::game {
// Disk-only evidence. Neither these files nor an attribute registration prove
// that the engine has streamed GPU/model/physics resources for native spawning.
struct GameAssetModel {
    std::string key;
    bool safeKey=false,geometry=false,textures=false;
    bool completePair() const { return geometry && textures; }
};
struct GameAssetInventory {
    bool rootKnown=false;
    bool globalA=false,globalB=false;
    bool track=false,streamTrack=false;
    std::vector<GameAssetModel> models;
    unsigned completeModels=0;
};
bool validCarFolderKey(std::string_view key);
GameAssetInventory inspectGameAssets(
    const std::filesystem::path& gameRoot,
    const std::vector<std::string_view>& keys);
} // namespace frr::game
