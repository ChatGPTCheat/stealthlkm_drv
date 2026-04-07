#ifndef STEALTH_SPE_H
#define STEALTH_SPE_H

#include <linux/types.h>

int init_spe_engine(void);
void set_spe_watchpoint(uint64_t start_va, uint64_t size, uint32_t pid);
void stop_spe_engine(void);
void cleanup_spe_engine(void);

#endif // STEALTH_SPE_H
