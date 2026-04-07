#ifndef STEALTH_SPE_ENGINE_H
#define STEALTH_SPE_ENGINE_H

#ifdef __KERNEL__
#include <linux/types.h>
#include <asm/sysreg.h>
#else
#include <stdint.h>
#include <stddef.h>
#endif

/*
 * ARM CoreSight SPE (Statistical Profiling Extension) Registers
 * Extracted from Deep Research hardware architecture report.
 * These registers allow us to hardware-profile memory accesses without using debug registers.
 */

#ifdef __KERNEL__
/* Sysreg encodings for SPE (kernel-only) */
#ifndef SYS_PMSCR_EL1
#define SYS_PMSCR_EL1   sys_reg(3, 0, 9, 9, 0)
#define SYS_PMBPTR_EL1   sys_reg(3, 0, 9, 9, 1)
#define SYS_PMSICR_EL1   sys_reg(3, 0, 9, 9, 2)
#define SYS_PMBSR_EL1    sys_reg(3, 0, 9, 9, 3)
#define SYS_PMSFCR_EL1   sys_reg(3, 0, 9, 9, 4)
#define SYS_PMSEVFR_EL1  sys_reg(3, 0, 9, 9, 5)
#define SYS_PMSLATFR_EL1 sys_reg(3, 0, 9, 9, 6)
#define SYS_PMBIDR_EL1   sys_reg(3, 0, 9, 10, 7)
#endif

/* PMSCR_EL1 bits (Control Register) */
#define PMSCR_EL1_E0SPE     (1U << 0)
#define PMSCR_EL1_E1SPE     (1U << 1)
#define PMSCR_EL1_CX        (1U << 3)
#define PMSCR_EL1_PA        (1U << 4)
#define PMSCR_EL1_TS        (1U << 5)

/* PMSFCR_EL1 bits (Filter Control Register) */
#define PMSFCR_EL1_LD       (1U << 0)
#define PMSFCR_EL1_ST       (1U << 1)

/* SPE Packet format parsing constants */
#define SPE_PKT_TYPE_MASK   0xC0
#define SPE_PKT_EVENT       0x40
#define SPE_PKT_ADDRESS     0x80
#define SPE_PKT_CONTEXT     0xC0

/* Address Packet classes */
#define SPE_ADDR_PKT_PC     0x00
#define SPE_ADDR_PKT_DATA   0x01
#endif /* __KERNEL__ */

/* Data structure shared between kernel and userspace */
struct spe_event_record {
    uint64_t target_vaddr;
    uint64_t pc_vaddr;
    uint8_t  is_write;
    uint64_t timestamp;
};

#endif /* STEALTH_SPE_ENGINE_H */
