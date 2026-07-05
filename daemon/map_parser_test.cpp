#include <cstdio>
#include <iostream>
#include <cassert>
#include <fstream>
#include "map_parser.h"

void create_mock_map(const std::string& filename, const std::string& content) {
    std::ofstream file(filename);
    file << content;
    file.close();
}

void test_module_not_found() {
    std::string mock_file = "mock_maps_not_found.txt";
    create_mock_map(mock_file,
        "10000000-20000000 r-xp 00000000 fd:00 123456 /path/to/other.so\n"
        "20000000-30000000 rw-p 00000000 fd:00 123456 /path/to/other.so\n"
    );

    uint64_t size = 12345;
    uint64_t base = MemoryMapParser::GetModuleBaseFromFile(mock_file, "target_module.so", &size);

    assert(base == 0);
    assert(size == 0);

    std::remove(mock_file.c_str());
    std::cout << "[+] test_module_not_found passed!" << std::endl;
}

void test_file_not_found() {
    uint64_t size = 12345;
    uint64_t base = MemoryMapParser::GetModuleBaseFromFile("non_existent_file.txt", "target_module.so", &size);

    assert(base == 0);
    assert(size == 0);

    std::cout << "[+] test_file_not_found passed!" << std::endl;
}

void test_module_found() {
    std::string mock_file = "mock_maps_found.txt";
    create_mock_map(mock_file,
        "10000000-11000000 r-xp 00000000 fd:00 123456 /path/to/target_module.so\n"
        "11000000-12000000 r--p 00000000 fd:00 123456 /path/to/target_module.so\n"
        "12000000-13000000 rw-p 00000000 fd:00 123456 /path/to/target_module.so\n"
        "20000000-30000000 rw-p 00000000 fd:00 123456 /path/to/other.so\n"
    );

    uint64_t size = 0;
    uint64_t base = MemoryMapParser::GetModuleBaseFromFile(mock_file, "target_module.so", &size);

    assert(base == 0x10000000);
    assert(size == (0x13000000 - 0x10000000));

    std::remove(mock_file.c_str());
    std::cout << "[+] test_module_found passed!" << std::endl;
}

int main() {
    std::cout << "Running MemoryMapParser tests..." << std::endl;

    test_module_not_found();
    test_file_not_found();
    test_module_found();

    std::cout << "All MemoryMapParser tests passed successfully!" << std::endl;
    return 0;
}
