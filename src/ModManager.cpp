#include "ModManager.hpp"

using namespace geode::prelude;

// Smallest optimization ever to not keep saveDirectory into memory after that runs
std::unordered_map<std::string, asp::fs::Path> ModManager::SaveFiles = []() {
    auto saveDirectory = dirs::getSaveDir();

    return std::unordered_map<std::string, asp::fs::Path>{
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