#include "memory_engine.h"

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <errno.h>
#include <string.h>

MemoryEngine::MemoryEngine(int target_pid) 
    : fd_(-1), target_pid_(target_pid), shm_(nullptr), is_valid_(false)
{
    // C++ Memory Allocation for Zero-Copy IPC
    shm_size_ = (sizeof(struct ShmData) + 4095) & ~4095;
    
    // ПРАВКА Oт SENIOR_REVERSE_ENGINEER:
    // Использовать MAP_SHARED | MAP_ANONYMOUS, чтобы избежать CoW (Copy-on-Write) багов
    // при первой записи в shm после отработки get_user_pages_fast ядром.
    shm_ = static_cast<struct ShmData*>(mmap(NULL, shm_size_, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0));
    
    if (shm_ == MAP_FAILED) {
        perror("[-] MemoryEngine: mmap failed");
        shm_ = nullptr;
        return;
    }
    
    memset(shm_, 0, shm_size_);

    fd_ = open("/dev/ashmem", O_RDWR);
    if (fd_ < 0) {
        perror("[-] MemoryEngine: Failed to open /dev/ashmem");
        munmap(shm_, shm_size_);
        shm_ = nullptr;
        return;
    }

    struct stealth_turbo_init init_req;
    init_req.target_pid = target_pid_;
    init_req.shared_vaddr = reinterpret_cast<uint64_t>(shm_);

    int ret = ioctl(fd_, STEALTH_IOCTL_START_TURBO, &init_req);
    if (ret < 0) {
        perror("[-] MemoryEngine: START_TURBO IOCTL failed");
        close(fd_);
        munmap(shm_, shm_size_);
        fd_ = -1;
        shm_ = nullptr;
        return;
    }

    // Wait for the kernel thread to come alive and set the magic signature
    usleep(10000); // 10ms wait
    if (shm_->magic_verify == 0xDEADC0DE) {
        is_valid_ = true;
    } else {
        printf("[-] MemoryEngine: Turbo loop was not connected successfully.\n");
    }
}

MemoryEngine::~MemoryEngine()
{
    if (is_valid_) {
        // Shut down the kernel polling thread gracefully
        if (fd_ >= 0) {
            ioctl(fd_, STEALTH_IOCTL_STOP_TURBO, 0);
        }
    }

    if (shm_) {
        munmap(shm_, shm_size_);
        shm_ = nullptr;
    }

    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
}

bool MemoryEngine::IsValid() const {
    return is_valid_;
}

bool MemoryEngine::ReadBatch(const std::vector<uint64_t>& addresses, std::vector<uint64_t>& results, uint32_t size_per_read)
{
    if (!is_valid_ || size_per_read > 16 || addresses.empty()) {
        return false;
    }

    results.clear();
    results.reserve(addresses.size());

    size_t total_items = addresses.size();
    size_t offset = 0;

    // Split reads into chunks of MAX_BATCH_ENTRIES
    while (offset < total_items) {
        size_t current_chunk_size = total_items - offset;
        if (current_chunk_size > MAX_BATCH_ENTRIES) {
            current_chunk_size = MAX_BATCH_ENTRIES;
        }

        shm_->batch_active = 1;
        shm_->batch_count = current_chunk_size;
        shm_->batch_delay_ns = 0; // Run as fast as possible

        // Populate the chunk buffer
        for (size_t i = 0; i < current_chunk_size; ++i) {
            shm_->batch[i].vaddr = addresses[offset + i];
            shm_->batch[i].size = size_per_read;
            shm_->batch[i].status = 0;
            shm_->batch[i].data[0] = 0; // optional clear
        }

        /* 
         * MEMORY BARRIER RELEASE 
         * Ensure all previous CPU stores to shm_->batch are visible in RAM 
         * before we signal the kernel thread that the sequence has incremented.
         */
        uint32_t next_seq = shm_->user_seq + 1;
        __atomic_store_n(&shm_->user_seq, next_seq, __ATOMIC_RELEASE);

        /*
         * SPIN WAIT ACQUIRE
         * Actively spin reading memory without syscalls until the kernel 
         * signals that it processed the frame.
         */
        while (__atomic_load_n(&shm_->kernel_seq, __ATOMIC_ACQUIRE) != next_seq) {
            // ARM64 Yield Hint (reduces power consumption during spin)
            asm volatile("yield");
        }

        // Collect results from the completed chunk
        for (size_t i = 0; i < current_chunk_size; ++i) {
            if (shm_->batch[i].status == 1) {
                // Read successful
                uint64_t value = *reinterpret_cast<uint64_t*>(shm_->batch[i].data);
                results.push_back(value);
            } else {
                // Translation or Memory Copy Failed
                results.push_back(0); 
            }
        }

        offset += current_chunk_size;
    }

    // Unset active batch
    shm_->batch_active = 0;
    
    return true;
}

bool MemoryEngine::SetHWBP(uint64_t address, int override_tid) {
    if (!is_valid_ || fd_ < 0) return false;

    struct stealth_hwbp_req req;
    req.target_pid = (override_tid == -1) ? target_pid_ : override_tid;
    req.vaddr = address;

    int ret = ioctl(fd_, STEALTH_IOCTL_SET_HWBP, &req);
    if (ret < 0) {
        perror("[-] MemoryEngine: SET_HWBP failed");
        return false;
    }
    return true;
}

bool MemoryEngine::RemoveHWBP() {
    if (!is_valid_ || fd_ < 0) return false;

    int ret = ioctl(fd_, STEALTH_IOCTL_REMOVE_HWBP, 0);
    return ret == 0;
}

struct StealthPtRegs MemoryEngine::GetDebugRegs() const {
    if (!is_valid_ || !shm_) {
        return {};
    }
    return shm_->debug_regs;
}

bool MemoryEngine::CheckDebugEvent(struct StealthPtRegs* out_regs) {
    if (!is_valid_ || !shm_) return false;
    
    // Non-blocking check for ATOMIC event
    if (__atomic_load_n(&shm_->debug_event_active, __ATOMIC_ACQUIRE) == 1) {
        if (out_regs) {
            *out_regs = shm_->debug_regs;
        }
        return true;
    }
    return false;
}

bool MemoryEngine::ResumeHWBP(const struct StealthPtRegs& modified_regs) {
    if (!is_valid_ || !shm_) return false;

    // Apply the user's tampered registers
    shm_->debug_regs = modified_regs;
    
    // Issue release and wake_up
    __atomic_store_n(&shm_->debug_resume_flag, 1, __ATOMIC_RELEASE);
    ioctl(fd_, STEALTH_IOCTL_RESUME_HWBP, 0);

    // Block exactly until the kernel removes the active flag
    while (__atomic_load_n(&shm_->debug_event_active, __ATOMIC_ACQUIRE) == 1) {
        asm volatile("yield");
    }

    return true;
}

// ===== SPE Hardware Profiling (Zero Detection HWBP) =====

void MemoryEngine::SetSPEWatchpoint(uint64_t vaddr, uint32_t size) {
    if (!is_valid_ || !shm_) return;

    printf("[SPE] Setting hardware watchpoint at 0x%lx (size: %u)\n",
           (unsigned long)vaddr, size);

    shm_->spe_hit_count = 0;
    memset((void*)shm_->spe_hits, 0, sizeof(shm_->spe_hits));

    shm_->cmd_vaddr = vaddr;
    shm_->cmd_len = size;
    __atomic_store_n(&shm_->cmd_type, 8, __ATOMIC_RELEASE);

    while (__atomic_load_n(&shm_->cmd_type, __ATOMIC_ACQUIRE) != 0) {
        usleep(100);
    }
    printf("[SPE] Watchpoint activated successfully.\n");
}

void MemoryEngine::RemoveSPEWatchpoint() {
    if (!is_valid_ || !shm_) return;

    __atomic_store_n(&shm_->cmd_type, 9, __ATOMIC_RELEASE);

    while (__atomic_load_n(&shm_->cmd_type, __ATOMIC_ACQUIRE) != 0) {
        usleep(100);
    }
    printf("[SPE] Watchpoint removed.\n");
}

bool MemoryEngine::PollSPEEvents() {
    if (!is_valid_ || !shm_) return false;

    uint32_t count = __atomic_load_n(&shm_->spe_hit_count, __ATOMIC_ACQUIRE);
    if (count > 0) {
        printf("[SPE-ALERT] Intercepted %u memory accesses!\n", count);
        for (uint32_t i = 0; i < count && i < 10; i++) {
            printf("  -> Hit %u: VA=0x%lx | PC=0x%lx | Write=%u | Time=%lu\n",
                   i,
                   (unsigned long)shm_->spe_hits[i].target_vaddr,
                   (unsigned long)shm_->spe_hits[i].pc_vaddr,
                   shm_->spe_hits[i].is_write,
                   (unsigned long)shm_->spe_hits[i].timestamp);
        }
        __atomic_store_n(&shm_->spe_hit_count, 0, __ATOMIC_RELEASE);
        return true;
    }
    return false;
}
