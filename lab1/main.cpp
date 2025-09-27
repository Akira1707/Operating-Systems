#include "MyDaemon.h"
#include <filesystem>
#include <iostream>
namespace fs = std::filesystem;

int main(int argc, char* argv[]) {
    if (argc < 2) { 
        std::cerr << "Usage: " << argv[0] << " <config-file>\n"; 
        return 1; 
    }
    std::string cfg = argv[1];
    if (!fs::exists(cfg)) { 
        std::cerr << "Config file " << cfg << " does not exist\n"; 
        return 1; 
    }
    return MyDaemon::instance().run(cfg);
}