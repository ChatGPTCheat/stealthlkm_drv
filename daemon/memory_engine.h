#pragma once

#include <cstdint>
#include <vector>
#include <sys/ioctl.h>
#include "shared_buffer.h"

class MemoryEngine {
public:
    explicit MemoryEngine(int target_pid);
    ~MemoryEngine();

    bool IsValid() const;

    /* 
     * Reads a batch of memory chunks directly via physical memory translation (bypassing syscalls).
     */
    bool ReadBatch(const std::vector<uint64_t>& addresses, std::vector<uint64_t>& results, uint32_t size_per_read);

    bool SetHWBP(uint64_t address, int override_tid = -1);
    bool RemoveHWBP();
    struct StealthPtRegs GetDebugRegs() const;

    /* 
     * Spin-waits for the Breakpoint to trigger. 
     * Once triggered, calls the provided callback where you can read/modify the registers.
     * Then signals the kernel to resume the thread smoothly.
     */
    template <typename Func>
    bool WaitAndProcessDebugEvent(Func callback) {
        if (!is_valid_ || !shm_) return false;

        // Поллим флажок активности
        while (__atomic_load_n(&shm_->debug_event_active, __ATOMIC_ACQUIRE) == 0) {
            asm volatile("yield");
        }

        // Передаем регистрам callback, юзерспейс может их изменить (например regs.regs[0] = 1;)
        callback(shm_->debug_regs);

        // Сигнализируем ядру "можно продолжать"
        __atomic_store_n(&shm_->debug_resume_flag, 1, __ATOMIC_RELEASE);
        
        // Будим wait_queue в ядре
        ioctl(fd_, STEALTH_IOCTL_RESUME_HWBP, 0);

        // Ждем, пока ядро не подтвердит, что оно ушло из обработчика
        while (__atomic_load_n(&shm_->debug_event_active, __ATOMIC_ACQUIRE) == 1) {
            asm volatile("yield");
        }
        return true;
    }

    // Non-blocking version for TCP daemon
    bool CheckDebugEvent(struct StealthPtRegs* out_regs);
    bool ResumeHWBP(const struct StealthPtRegs& modified_regs);

private:
    int fd_;
    int target_pid_;
    struct ShmData* shm_;
    size_t shm_size_;
    bool is_valid_;
};
