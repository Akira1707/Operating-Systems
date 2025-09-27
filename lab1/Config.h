#pragma once
#include <filesystem>
#include <vector>
#include <string>

#include "utils.h"

namespace fs = std::filesystem;

struct ConfigEntry {
    fs::path folder;
    std::string ext; // without leading dot
};

class Config {
public:
    Config() = default;
    bool load_from(const fs::path& cfgpath);
    std::vector<ConfigEntry> get_entries() const;

private:
    std::vector<ConfigEntry> entries;
};