#include "map_parser.h"
#include <fstream>
#include <sstream>
#include <iostream>

uint64_t MemoryMapParser::GetModuleBase(pid_t pid, const std::string& module_name, uint64_t* out_size) {
    std::string maps_path = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream file(maps_path);
    if (!file.is_open()) {
        std::cerr << "[-] MemoryMapParser: Failed to open " << maps_path << "\n";
        return 0;
    }

    std::string line;
    uint64_t lowest_address = SIZE_MAX;
    uint64_t highest_address = 0;
    bool found = false;

    while (std::getline(file, line)) {
        if (line.find(module_name) != std::string::npos) {
            uint64_t start = 0, end = 0;
            // Expected format: "12345000-12346000 r-xp 00000000 fd:00 123456 path/to/module.so"
            if (sscanf(line.c_str(), "%lx-%lx", &start, &end) == 2) {
                if (!found) {
                    lowest_address = start;
                    highest_address = end;
                    found = true;
                } else {
                    // Always try to find the lowest possible base address and the highest end address 
                    // representing the complete bounding box of the contiguous mapping.
                    if (start < lowest_address) lowest_address = start;
                    if (end > highest_address) highest_address = end;
                }
            }
        }
    }

    if (found) {
        if (out_size) {
            *out_size = highest_address - lowest_address;
        }
        return lowest_address;
    }

    return 0;
}
