#ifndef _ARCH_ARM64_H
#define _ARCH_ARM64_H

#include <linux/types.h>

/* Memory Barriers */
#define dmb(opt)    asm volatile("dmb " #opt : : : "memory")
#define dsb(opt)    asm volatile("dsb " #opt : : : "memory")
#define isb()       asm volatile("isb" : : : "memory")

#ifndef read_sysreg
#define read_sysreg(reg) ({                 \
    unsigned long __val;                    \
    asm volatile("mrs %0, " #reg : "=r" (__val)); \
    __val;                                  \
})
#endif

#ifndef write_sysreg
#define write_sysreg(val, reg) do {         \
    unsigned long __val = (unsigned long)(val); \
    asm volatile("msr " #reg ", %0" : : "r" (__val)); \
} while (0)
#endif

static inline void enable_pan(void) {
    asm volatile("msr pan, #1" : : : "memory");
}

static inline void disable_pan(void) {
    asm volatile("msr pan, #0" : : : "memory");
}

#define xpaclri()   asm volatile("hint #7" : : : "memory")
#define bti_c()     asm volatile("hint #34" : : : "memory")
#define bti_j()     asm volatile("hint #36" : : : "memory")
#define bti_jc()    asm volatile("hint #38" : : : "memory")

static inline void ic_ivau(unsigned long addr) {
    asm volatile("ic ivau, %0" : : "r" (addr) : "memory");
}

static inline void dc_cvau(unsigned long addr) {
    asm volatile("dc cvau, %0" : : "r" (addr) : "memory");
}

static inline void tlbi_vaae1is(unsigned long addr) {
    asm volatile("tlbi vaae1is, %0" : : "r" (addr) : "memory");
}

static inline void tlbi_vmalle1is(void) {
    asm volatile("tlbi vmalle1is" : : : "memory");
}

#define _ASM_EXTABLE(from, to)                  \
    ".pushsection __ex_table, \"a\"\n"          \
    ".align 3\n"                                \
    ".long (" #from " - .), (" #to " - .)\n"    \
    ".popsection\n"

#endif /* _ARCH_ARM64_H */
