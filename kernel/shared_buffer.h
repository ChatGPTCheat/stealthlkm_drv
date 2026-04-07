#ifndef SHARED_BUFFER_H
#define SHARED_BUFFER_H

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#include "spe_engine.h"

#define MAX_CMD_DATA_SIZE 262144
#define MAX_BATCH_ENTRIES 64

struct StealthPtRegs {
    uint64_t regs[31];
    uint64_t sp;
    uint64_t pc;
    uint64_t pstate;
};

struct BatchEntry {
    uint64_t vaddr;
    uint32_t size;
    uint32_t status;
    uint8_t data[16];
};

struct ShmData {
    uint32_t magic_verify;
    uint32_t user_seq;
    uint32_t kernel_seq;
    uint32_t turbo_counter;

    /* HWBP Debugging Interface */
    volatile uint32_t debug_lock;
    volatile uint32_t debug_event_active;
    volatile uint32_t debug_resume_flag;
    /* SPE Hardware Profiling Interface */
    volatile uint32_t spe_hit_count;
    struct spe_event_record spe_hits[10];
    struct StealthPtRegs debug_regs;

    /* Single Command Interface */
    uint32_t cmd_type;
    uint64_t cmd_vaddr;
    uint64_t cmd_paddr_out;
    uint32_t cmd_len;
    uint8_t cmd_data[MAX_CMD_DATA_SIZE];

    /* Batch Interface */
    uint32_t batch_active;
    uint32_t batch_count;
    uint32_t batch_delay_ns;
    struct BatchEntry batch[MAX_BATCH_ENTRIES];
};

/* IOCTL Definitions */
#define STEALTH_MAGIC 's'

struct stealth_turbo_init {
    int target_pid;
    uint64_t shared_vaddr;
};

struct stealth_hwbp_req {
    int target_pid;
    uint64_t vaddr;
};

#define MAX_PATTERN_LEN 128

struct AobScanRequest {
    uint64_t start_vaddr;
    uint64_t scan_size;
    uint32_t pattern_len;
    uint8_t  pattern[MAX_PATTERN_LEN];
    uint8_t  mask[MAX_PATTERN_LEN];
};

struct stealth_scan_req {
    int target_pid;
    uint64_t found_vaddr;
};

#define STEALTH_IOCTL_PING _IO(STEALTH_MAGIC, 1)
#define STEALTH_IOCTL_START_TURBO _IOW(STEALTH_MAGIC, 2, struct stealth_turbo_init)
#define STEALTH_IOCTL_STOP_TURBO  _IO(STEALTH_MAGIC, 3)
#define STEALTH_IOCTL_SET_HWBP    _IOW(STEALTH_MAGIC, 4, struct stealth_hwbp_req)
#define STEALTH_IOCTL_REMOVE_HWBP _IO(STEALTH_MAGIC, 5)
#define STEALTH_IOCTL_RESUME_HWBP _IO(STEALTH_MAGIC, 6)
#define STEALTH_IOCTL_SCAN_PATTERN _IOWR(STEALTH_MAGIC, 7, struct stealth_scan_req)

#endif /* SHARED_BUFFER_H */
