#include <iostream>
#include <vector>
#include <chrono>
#include <thread>
#include <string>
#include <cstdio>
#include <unistd.h>

#include <sys/syscall.h>
#include <atomic>

#include "memory_engine.h"
#include "map_parser.h"

// Simple dummy PID resolver
int GetPidByName(const std::string& name) {
    std::string cmd = "pidof " + name;
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return -1;

    char buffer[128] = {0};
    if (fgets(buffer, 128, pipe)) {
        pclose(pipe);
        return std::stoi(buffer);
    }
    pclose(pipe);
    return -1;
}

// Function to test HWBP
__attribute__((noinline)) void DummyTriggerFunction() {
    asm volatile("nop");
    asm volatile("nop");
    std::cout << "[*] DummyTriggerFunction: I'm executing and doing completely normal things..." << std::endl;
}

int main(int argc, char** argv) {
    std::string target_name = "com.proximabeta.mf.uamo";
    int pid = -1;
    bool is_self = false;

    if (argc > 1) {
        if (std::string(argv[1]) == "self") {
            pid = getpid();
            is_self = true;
            std::cout << "[*] Forced Self-Test Mode on PID: " << pid << std::endl;
        } else {
            pid = std::stoi(argv[1]);
            std::cout << "[*] Using PID from arguments: " << pid << std::endl;
        }
    } else {
        pid = GetPidByName(target_name);
        if (pid > 0) {
            std::cout << "[*] Found " << target_name << " at PID: " << pid << std::endl;
        } else {
            pid = getpid(); // Test on ourselves
            is_self = true;
            std::cout << "[-] Target not found. Connecting to Self PID: " << pid << std::endl;
        }
    }

    std::cout << "[*] Initializing Memory Engine..." << std::endl;
    MemoryEngine engine(pid);

    if (!engine.IsValid()) {
        std::cerr << "[-] Failed to initialize Memory Engine." << std::endl;
        return EXIT_FAILURE;
    }
    std::cout << "[+] Engine successfully injected and ready!" << std::endl;

    if (is_self) {
        auto test_addr = (uint64_t)&DummyTriggerFunction;
        std::cout << "\n[***] HWBP FREEZE TEST [***]" << std::endl;
        std::cout << "[*] Getting Thread ID to attach Breakpoint..." << std::endl;

        std::atomic<int> dummy_tid = -1;

        std::thread dummy_runner([&dummy_tid](){
            dummy_tid = syscall(SYS_gettid);
            std::cout << "[Thread] TID " << dummy_tid << " ready, sleeping 2s before executing function..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));
            std::cout << "[Thread] About to call DummyTriggerFunction..." << std::endl;
            DummyTriggerFunction();
            std::cout << "[Thread] DummyTriggerFunction returned." << std::endl;
        });

        // Ждем запуска потока
        while (dummy_tid == -1) { std::this_thread::yield(); }
        
        std::cout << "[*] Setting HWBP at DummyTriggerFunction (0x" 
                  << std::hex << test_addr << std::dec << ") for TID: " << dummy_tid << std::endl;

        // Важно: в Linux HWBP прикрепляются к конкретному Потоку (task_struct / LWP), а не ко всему PID.
        // Поэтому для теста мы используем TID дочернего потока!
        if (engine.SetHWBP(test_addr, dummy_tid)) {
            std::cout << "[+] Breakpoint set! Thread will execute it in 2s..." << std::endl;
            std::cout << "[*] Polling for breakpoint hit (snapshot-only mode)..." << std::endl;

            // Poll for breakpoint hit (non-blocking, snapshot-only)
            bool hit = false;
            struct StealthPtRegs captured_regs;

            for (int i = 0; i < 60; i++) { // 60 * 100ms = 6 seconds
                if (engine.CheckDebugEvent(&captured_regs)) {
                    hit = true;
                    std::cout << "\n[!!!] BREAKPOINT HIT (SNAPSHOT CAPTURED) [!!!]" << std::endl;
                    std::cout << "      -> Captured PC: 0x" << std::hex << captured_regs.pc << std::dec << std::endl;
                    std::cout << "      -> Captured SP: 0x" << std::hex << captured_regs.sp << std::dec << std::endl;
                    std::cout << "      -> Captured X0: 0x" << std::hex << captured_regs.regs[0] << std::dec << std::endl;
                    std::cout << "[*] Note: Thread was NOT frozen (snapshot-only mode for NMI safety)" << std::endl;
                    break;
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            if (!hit) {
                std::cout << "[-] Breakpoint did not trigger within 6 second timeout." << std::endl;
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            engine.RemoveHWBP();
        } else {
            std::cout << "[-] Failed to set HWBP!" << std::endl;
        }
        dummy_runner.join();
        std::cout << "[*] Dummy thread finished successfully without crashing!" << std::endl;
    } else {
        std::cout << "\n[*] Connected to Game. Engine Loop starting..." << std::endl;

        // ===== Find libUE4.so base =====
        uint64_t ue4_size = 0;
        uint64_t ue4_base = MemoryMapParser::GetModuleBase(pid, "libUE4.so", &ue4_size);
        if (ue4_base > 0) {
            std::cout << "[+] libUE4.so base: 0x" << std::hex << ue4_base
                      << " | size: 0x" << ue4_size << std::dec
                      << " (" << (ue4_size / 1024 / 1024) << " MB)" << std::endl;
        } else {
            std::cout << "[-] libUE4.so not found in /proc/" << pid << "/maps" << std::endl;
        }

        // ===== SPE Hardware Profiling Test =====
        std::cout << "\n--- SPE Hardware Profiling Test ---" << std::endl;

        uint64_t spe_addr = ue4_base; // default to libUE4.so base
        if (spe_addr == 0) {
            // If no libUE4.so, use a generic address from the process
            std::cout << "[*] No libUE4.so found, using generic test address..." << std::endl;
            spe_addr = 0x1000; // Just a placeholder for testing
        }

        std::cout << "[*] Monitoring address: 0x" << std::hex << spe_addr << std::dec << std::endl;

        engine.SetSPEWatchpoint(spe_addr, 256);

        std::cout << "[*] SPE is now monitoring 0x" << std::hex << spe_addr << std::dec << std::endl;
        std::cout << "[*] Polling for memory access events... (Auto-exit after 10 seconds)" << std::endl;

        int poll_count = 0;
        while (poll_count < 200) { // 200 * 50ms = 10 seconds
            if (engine.PollSPEEvents()) {
                std::cout << "[*] Events consumed. Continuing to monitor..." << std::endl;
            }
            usleep(50000); // 50ms poll interval
            poll_count++;
        }

        engine.RemoveSPEWatchpoint();
    }

    std::cout << "[*] Clean Exit." << std::endl;
    return 0;
}
