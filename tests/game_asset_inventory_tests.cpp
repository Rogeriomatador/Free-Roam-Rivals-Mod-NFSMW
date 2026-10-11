#include "game/GameAssetInventory.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#ifdef _WIN32
#include <windows.h>
#endif
using namespace frr::game;
static void require(bool yes,const char* label){if(!yes){std::cerr<<label<<'\n';std::exit(1);}}
static void file(const std::filesystem::path& path,bool nonempty=true) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream stream(path,std::ios::binary|std::ios::trunc);
    if(nonempty) stream<<"test";
}
int main(){
    require(validCarFolderKey("gti")&&validCarFolderKey("lancerevo8"),"safe lowercase keys");
    require(!validCarFolderKey("")&&!validCarFolderKey("../evil")&&
        !validCarFolderKey("GTI")&&!validCarFolderKey("car/name")&&
        !validCarFolderKey(std::string(49,'a')),"reject traversal/invalid names");
    auto root=std::filesystem::temp_directory_path();
#ifdef _WIN32
    root/="frr-game-asset-"+std::to_string(GetCurrentProcessId());
#else
    root/="frr-game-asset-test";
#endif
    std::error_code err;
    std::filesystem::remove_all(root,err);
    auto none=inspectGameAssets(root,{"gti"});
    require(!none.rootKnown&&none.models.empty(),"missing game root yields no proof");
    file(root/"CARS"/"GTI"/"GEOMETRY.BIN");
    file(root/"CARS"/"GTI"/"TEXTURES.BIN");
    file(root/"CARS"/"RX8"/"GEOMETRY.BIN",false);
    file(root/"GLOBAL"/"GLOBALB.LZC");
    file(root/"TRACKS"/"L2RA.BUN");
    auto inventory=inspectGameAssets(root,{"gti","rx8","../../escape","lancerevo8"});
    require(inventory.rootKnown&&inventory.globalB&&inventory.track&&!inventory.globalA&&!inventory.streamTrack,
        "world and globals presence is independently measured");
    require(inventory.models.size()==4&&inventory.completeModels==1,"only fully present, nonempty pairs counted");
    require(inventory.models[0].safeKey&&inventory.models[0].completePair(),"GTI resource pair");
    require(inventory.models[1].safeKey&&!inventory.models[1].geometry&&!inventory.models[1].textures,
        "empty geometry is not accepted");
    require(!inventory.models[2].safeKey&&!inventory.models[2].completePair(),"traversal cannot resolve");
    require(inventory.models[3].safeKey&&!inventory.models[3].completePair(),"missing model remains unknown");
    // Inventory is read-only: all known files survive untouched.
    require(std::filesystem::file_size(root/"CARS"/"GTI"/"GEOMETRY.BIN")==4,
        "asset inventory must never mutate original game files");
    std::filesystem::remove_all(root,err);
    std::cout<<"Game asset inventory: retail paths, safe keys, zero-byte, incomplete, and no write passed\n";
}
