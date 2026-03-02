#pragma once

class ModManager {
public:
    static GameManager* GameManager;
    static LocalLevelManager* LocalLevelManager;

    static std::unordered_map<std::string, std::filesystem::path> SaveFiles;

    static bool DisableWarning;
};