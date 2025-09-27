#include "Config.h"
#include <fstream>
#include <sstream>
#include <iostream>

bool Config::load_from(const fs::path& cfgpath) {
    std::vector<ConfigEntry> tmp;
    std::ifstream ifs(cfgpath);
    if (!ifs.is_open()) {
        safe_syslog(LOG_ERR, "Cannot open config file %s", cfgpath.c_str());
        return false;
    }
    std::string line;
    int lineno = 0;
    while (std::getline(ifs, line)) {
        ++lineno;
        auto start = line.find_first_not_of(" \t\r\n");
        if (start == std::string::npos) continue;
        auto end = line.find_last_not_of(" \t\r\n");
        std::string s = line.substr(start, end - start + 1);
        if (s.empty() || s[0] == '#') continue;

        std::istringstream iss(s);
        std::string folder, ext;
        if (!(iss >> folder >> ext)) {
            safe_syslog(LOG_WARNING, "Invalid config line %d (ignored)", lineno);
            continue;
        }
        if (!ext.empty() && ext.front() == '.') ext.erase(0,1);

        try {
            fs::path p = fs::absolute(fs::path(folder));
            if (!fs::exists(p) || !fs::is_directory(p)) {
                safe_syslog(LOG_WARNING, "Config line %d: folder %s not exist or not a directory (ignored)", lineno, p.c_str());
                continue;
            }
            tmp.push_back(ConfigEntry{p, ext});
        } catch (const std::exception& e) {
            safe_syslog(LOG_WARNING, "Config line %d: exception: %s (ignored)", lineno, e.what());
            continue;
        }
    }
    entries.swap(tmp);
    return true;
}

std::vector<ConfigEntry> Config::get_entries() const {
    return entries;
}