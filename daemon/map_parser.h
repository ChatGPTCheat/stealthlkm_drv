#pragma once
#include <string>
#include <cstdint>

class MemoryMapParser {
public:
    // Returns the lowest base address for the given contiguous module mappings.
    // Optionally outputs the total combined mapping size for the module.
    static uint64_t GetModuleBase(pid_t pid, const std::string& module_name, uint64_t* out_size = nullptr);

    // Dependency injection friendly version for testing
    static uint64_t GetModuleBaseByPath(const std::string& maps_path, const std::string& module_name, uint64_t* out_size = nullptr);
};
