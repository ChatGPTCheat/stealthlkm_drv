#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/dma-mapping.h>
#include <linux/interrupt.h>
#include <linux/workqueue.h>
#include <asm/sysreg.h>

#include "spe_engine.h"
#include "shared_buffer.h"

#define SPE_BUFFER_SIZE (256 * 1024) // 256 KB Ring Buffer (safe order 6)

extern void *g_shared_buffer; // From stealth_analyzer.c

struct spe_stealth_monitor {
    void *vaddr;
    dma_addr_t paddr;
    size_t size;
    int irq_num;
    struct work_struct parse_work;
    
    uint64_t target_va_start;
    uint64_t target_va_end;
    uint32_t target_pid;
    bool is_active;
};

static struct spe_stealth_monitor spe_mon;

static bool spe_hw_supported(void) {
    u64 aa64dfr0;
    asm volatile("mrs %0, id_aa64dfr0_el1" : "=r" (aa64dfr0));
    // PMSVer field is bits [35:32], non-zero means SPE is present
    return ((aa64dfr0 >> 32) & 0xF) != 0;
}

static int configure_spe_hardware(void) {
    u64 pmscr, pmsfcr, pmsicr, pmsirr, pmblimitr;

    if (!spe_hw_supported()) {
        pr_warn("Stealth LKM: SPE not supported on this CPU, skipping hardware config\n");
        return -ENODEV;
    }

    // 1. Interval Registers (1024 ops, enable jitter)
    pmsirr = 0x400 | 1;
    asm volatile("msr s3_0_c9_c9_3, %0" : : "r" (pmsirr));

    pmsicr = 0x400;
    asm volatile("msr s3_0_c9_c9_2, %0" : : "r" (pmsicr));

    // 2. Filter Registers (Load and Store only)
    pmsfcr = 0;
    pmsfcr |= (1ULL << 1); // FT
    pmsfcr |= (1ULL << 4); // LD
    pmsfcr |= (1ULL << 5); // ST
    asm volatile("msr s3_0_c9_c9_4, %0" : : "r" (pmsfcr));

    // 3. Buffer Pointers
    asm volatile("msr s3_0_c9_c10_1, %0" : : "r" (spe_mon.paddr));

    pmblimitr = (u64)(spe_mon.paddr + spe_mon.size);
    pmblimitr |= (1ULL << 0); // E bit
    asm volatile("msr s3_0_c9_c10_0, %0" : : "r" (pmblimitr));

    // 4. Enable Sampling at EL0
    pmscr = 0;
    pmscr |= (1ULL << 0); // E0SPE
    pmscr |= (1ULL << 3); // CX
    pmscr |= (1ULL << 4); // PA
    asm volatile("msr s3_0_c9_c9_0, %0" : : "r" (pmscr));

    asm volatile("isb");
    pr_info("Stealth LKM: SPE hardware configured successfully\n");
    return 0;
}

static void parse_spe_telemetry_stream(struct work_struct *work) {
    if (!spe_mon.is_active || !g_shared_buffer) return;
    struct ShmData *shm = (struct ShmData *)g_shared_buffer;
    
    unsigned char *buf = (unsigned char *)spe_mon.vaddr;
    size_t offset = 0;
    u32 current_pid = 0;

    while (offset < spe_mon.size) {
        unsigned char header = buf[offset];
        
        if (header == SPE_PKT_CONTEXT) { // 0x24 (Context ID)
            if (offset + 5 > spe_mon.size) break;
            memcpy(&current_pid, &buf[offset + 1], sizeof(u32));
            offset += 5;
            continue;
        }
        
        // 0xb2 -> Extended Address Packet (Data VA)
        if (header == 0xb2) { 
            if (offset + 9 > spe_mon.size) break;
            u64 payload = 0;
            memcpy(&payload, &buf[offset + 1], sizeof(u64));
            
            // Scrub MTE/PAC tags
            u64 virtual_address = payload & 0x00FFFFFFFFFFFFFFULL;
            
            if (current_pid == spe_mon.target_pid &&
                virtual_address >= spe_mon.target_va_start && 
                virtual_address <= spe_mon.target_va_end) {
                
                // Write hit to shared memory
                if (shm->spe_hit_count < 10) {
                    shm->spe_hits[shm->spe_hit_count].target_vaddr = virtual_address;
                    shm->spe_hits[shm->spe_hit_count].pc_vaddr = 0; // Will be extracted from PC packet
                    shm->spe_hits[shm->spe_hit_count].timestamp = ktime_get_ns();
                    shm->spe_hit_count++;
                }
            }
            offset += 9;
            continue;
        }
        
        if (header == 0x00) { offset += 1; continue; }
        if (header == 0x42) { offset += 5; continue; } // Events packet
        
        // Failsafe
        offset++; 
    }
}

irqreturn_t spe_buffer_full_isr(int irq, void *dev_id) {
    u64 pmbsr;
    asm volatile("mrs %0, s3_0_c9_c10_3" : "=r" (pmbsr));
    
    if ((pmbsr & (1ULL << 17)) && ((pmbsr & 0x3F) == 0x01)) {
        u64 pmblimitr;
        asm volatile("mrs %0, s3_0_c9_c10_0" : "=r" (pmblimitr));
        pmblimitr &= ~(1ULL << 0);
        asm volatile("msr s3_0_c9_c10_0, %0" : : "r" (pmblimitr));
        
        schedule_work(&spe_mon.parse_work);
        
        asm volatile("msr s3_0_c9_c10_1, %0" : : "r" (spe_mon.paddr));
        asm volatile("msr s3_0_c9_c10_3, xzr"); 
        
        pmblimitr |= (1ULL << 0);
        asm volatile("msr s3_0_c9_c10_0, %0" : : "r" (pmblimitr));
        asm volatile("isb");

        return IRQ_HANDLED;
    }
    return IRQ_NONE;
}

static bool spe_initialized = false;

int init_spe_engine(void) {
    if (spe_initialized) return 0;

    spe_mon.size = SPE_BUFFER_SIZE;

    // Allocate physically contiguous memory (256KB = 64 pages = order 6)
    unsigned int order = get_order(SPE_BUFFER_SIZE);

    if (order > 8) {
        pr_err("Stealth LKM: SPE buffer too large (order %u), refusing to allocate\n", order);
        return -ENOMEM;
    }

    spe_mon.vaddr = (void *)__get_free_pages(GFP_KERNEL | __GFP_ZERO | __GFP_NOWARN, order);

    if (!spe_mon.vaddr) {
        pr_err("Stealth LKM: SPE alloc_pages failed (order %u)\n", order);
        return -ENOMEM;
    }

    spe_mon.paddr = virt_to_phys(spe_mon.vaddr);

    INIT_WORK(&spe_mon.parse_work, parse_spe_telemetry_stream);
    spe_mon.is_active = false;
    spe_initialized = true;

    pr_info("Stealth LKM: SPE engine initialized (Buffer: %zu bytes, order: %u)\n", spe_mon.size, order);
    return 0;
}

void set_spe_watchpoint(uint64_t start_va, uint64_t size, uint32_t pid) {
    if (!spe_initialized) {
        int ret = init_spe_engine();
        if (ret < 0) {
            pr_err("Stealth LKM: SPE init failed in set_spe_watchpoint (%d)\n", ret);
            return;
        }
    }

    spe_mon.target_va_start = start_va;
    spe_mon.target_va_end = start_va + size;
    spe_mon.target_pid = pid;
    spe_mon.is_active = true;
    configure_spe_hardware();
}

void stop_spe_engine(void) {
    spe_mon.is_active = false;
    if (!spe_hw_supported()) return;
    u64 pmblimitr;
    asm volatile("mrs %0, s3_0_c9_c10_0" : "=r" (pmblimitr));
    pmblimitr &= ~(1ULL << 0);
    asm volatile("msr s3_0_c9_c10_0, %0" : : "r" (pmblimitr));
}

void cleanup_spe_engine(void) {
    spe_mon.is_active = false;
    if (spe_initialized && spe_hw_supported()) {
        u64 pmblimitr = 0;
        /* Disable SPE buffer - clear E bit */
        asm volatile("mrs %0, s3_0_c9_c10_0" : "=r" (pmblimitr));
        pmblimitr &= ~(1ULL << 0);
        asm volatile("msr s3_0_c9_c10_0, %0" : : "r" (pmblimitr));
        asm volatile("isb");
    }
    /* Cancel pending work without waiting - safe for module unload */
    if (spe_initialized) {
        cancel_work_sync(&spe_mon.parse_work);
        if (spe_mon.vaddr) {
            free_pages((unsigned long)spe_mon.vaddr, get_order(spe_mon.size));
            spe_mon.vaddr = NULL;
        }
        spe_initialized = false;
    }
}
