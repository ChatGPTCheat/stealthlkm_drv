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
            // Ждем пока главный поток не установит брейкпоинт на наш TID
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            DummyTriggerFunction();
        });

        // Ждем запуска потока
        while (dummy_tid == -1) { std::this_thread::yield(); }
        
        std::cout << "[*] Setting HWBP at DummyTriggerFunction (0x" 
                  << std::hex << test_addr << std::dec << ") for TID: " << dummy_tid << std::endl;

        // Важно: в Linux HWBP прикрепляются к конкретному Потоку (task_struct / LWP), а не ко всему PID.
        // Поэтому для теста мы используем TID дочернего потока!
        if (engine.SetHWBP(test_addr, dummy_tid)) {
            std::cout << "[+] Breakpoint set! Thread will execute it in 500ms..." << std::endl;
            std::cout << "[*] Engine is spinning and waiting for Kernel NMI..." << std::endl;
            
            bool hit = engine.WaitAndProcessDebugEvent([test_addr](struct StealthPtRegs& regs) {
                std::cout << "\n[!!!] THREAD FROZEN IN KERNEL SPACE [!!!]" << std::endl;
                std::cout << "      -> Captured PC: 0x" << std::hex << regs.pc << std::dec << std::endl;
                std::cout << "      -> Captured SP: 0x" << std::hex << regs.sp << std::dec << std::endl;
                std::cout << "      -> Captured X0: 0x" << std::hex << regs.regs[0] << std::dec << std::endl;
                
                regs.regs[0] = 0x1337BEEF; 
                std::cout << "[*] Modifying X0 to 0x1337BEEF before giving RESUME signal..." << std::endl;
                
                // ЭЛЕГАНТНЫЙ ОБХОД НА УРОВНЕ РЕГИСТРОВ (EARLY RETURN)
                // Так как брейкпоинт стоит на самой первой инструкции функции, если мы сделаем PC+=4,
                // мы пропустим критический пролог (stp x29,x30), что сломает стек и вызовет Segfault.
                // Вместо этого мы принудительно выходим из функции без ее выполнения!
                // Записываем в PC значение регистра X30 (Link Register), куда процессор должен был вернуться.
                regs.pc = regs.regs[30];
                
                // И сбрасываем флаг Single-Step (DBG_SPSR_SS = 1 << 21) внутри PSTATE
                regs.pstate &= ~(1ULL << 21);
            });
            
            if (hit) {
                std::cout << "\n[+] Kernel accepted Resume signal. Target thread unfrozen." << std::endl;
            } else {
                std::cout << "[-] Failed to capture the breakpoint event." << std::endl;
            }

            // Дадим дочернему потоку время штатно проснуться, выполнить Hardware Single Step
            // и завершить инструкцию до того, как мы жестко удалим HWBP структуру из ядра!
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            engine.RemoveHWBP();
        }
        dummy_runner.join();
        std::cout << "[*] Dummy thread finished successfully without crashing!" << std::endl;
    } else {
        std::cout << "\n[*] Connected to Game. Engine Loop starting..." << std::endl;
        // In the future you would read libs and set breakpoints here
    }

    std::cout << "[*] Clean Exit." << std::endl;
    return 0;
}
