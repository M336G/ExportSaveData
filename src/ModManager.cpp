#include "ModManager.h"

using namespace geode::prelude;

GameManager* ModManager::GameManager = GameManager::sharedState();
LocalLevelManager* ModManager::LocalLevelManager = LocalLevelManager::sharedState();

// Smallest optimization ever to not keep saveDirectory into memory after that runs
std::unordered_map<std::string, std::filesystem::path> ModManager::SaveFiles = []() {
    auto saveDirectory = dirs::getSaveDir();

    return std::unordered_map<std::string, std::filesystem::path>{
        { "CCGameManager.dat", saveDirectory / "CCGameManager.dat" },
        { "CCLocalLevels.dat", saveDirectory / "CCLocalLevels.dat" }
    };
}();

bool ModManager::DisableWarning = Mod::get()->getSettingValue<bool>("disable-warning");

$execute {
    listenForSettingChanges<bool>("disable-warning", [](bool enabled) {
        ModManager::DisableWarning = enabled;
    });
};