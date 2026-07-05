#include "map_parser.h"
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>

void CreateFakeMapsFile(const std::string& path, const std::string& content) {
    std::ofstream out(path);
    out << content;
    out.close();
}

int main() {
    std::string test_file = "/tmp/fake_maps";

    // Test 1: Single mapping
    std::string content1 =
        "10000000-10001000 r-xp 00000000 fd:00 123456 /data/app/com.example/libUE4.so\n"
        "10001000-10002000 rw-p 00001000 fd:00 123456 /data/app/com.example/libUE4.so\n";
    CreateFakeMapsFile(test_file, content1);

    uint64_t size = 0;
    uint64_t base = MemoryMapParser::GetModuleBaseByPath(test_file, "libUE4.so", &size);
    assert(base == 0x10000000);
    assert(size == 0x2000);
    std::cout << "[+] Test 1 passed\n";

    // Test 2: Multiple non-contiguous blocks but treated as one bounding box
    std::string content2 =
        "20000000-20001000 r-xp 00000000 fd:00 123456 /data/app/com.example/libtest.so\n"
        "20002000-20005000 r-xp 00000000 fd:00 123456 /data/app/com.example/libtest.so\n";
    CreateFakeMapsFile(test_file, content2);

    size = 0;
    base = MemoryMapParser::GetModuleBaseByPath(test_file, "libtest.so", &size);
    assert(base == 0x20000000);
    assert(size == 0x5000);
    std::cout << "[+] Test 2 passed\n";

    // Test 3: Module not found
    size = 0;
    base = MemoryMapParser::GetModuleBaseByPath(test_file, "nonexistent.so", &size);
    assert(base == 0);
    assert(size == 0);
    std::cout << "[+] Test 3 passed\n";

    // Test 4: File doesn't exist
    size = 0;
    base = MemoryMapParser::GetModuleBaseByPath("/tmp/does_not_exist_maps", "libtest.so", &size);
    assert(base == 0);
    assert(size == 0);
    std::cout << "[+] Test 4 passed\n";

    // Remove temp file
    remove(test_file.c_str());

    std::cout << "[+] All tests passed successfully!\n";
    return 0;
}
