#pragma once

class ModManager {
public:
    static std::unordered_map<std::string, asp::fs::Path> SaveFiles;
    static bool DisableWarning;
};