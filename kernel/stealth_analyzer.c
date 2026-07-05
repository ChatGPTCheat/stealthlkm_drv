// SPDX-License-Identifier: GPL-2.0
/*
 * stealth_analyzer.c — Kprobe Resolver + ASHMEM Kretprobe instance hook + Fast IPC
 *
 * 1. Ищет скрытые символы (kallsyms_lookup_name) через Kprobe.
 * 2. Хукает ashmem через Kretprobe на `ashmem_open`.
 * 3. При получении секретной ioctl-команды стартует `poly_turbo` — 
 *    поток для Zero-Copy IPC прямого чтения памяти без сисколов.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/kprobes.h>
#include <linux/kthread.h>
#include <linux/mm.h>
#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/fs.h>
#include <linux/vmalloc.h>
#include <linux/miscdevice.h>
#include <linux/sched/task.h>
#include <linux/sched/mm.h>
#include <linux/hw_breakpoint.h>
#include <linux/perf_event.h>
#include <linux/rcupdate.h>
#include <linux/ktime.h>
#include <linux/task_work.h>
#include <linux/wait.h>
#include <asm/sysreg.h>
#include <asm/neon.h>

#include "arch_arm64.h"
#include "shared_buffer.h"
#include "stealth_spe.h"

extern uint64_t neon_scan_16byte_pattern(const uint8_t* memory, uint64_t size, 
                                  const uint8_t* pattern, const uint8_t* mask, 
                                  uint32_t pattern_len);

static inline uint64_t safe_translate_el0(uint64_t vaddr) {
    uint64_t par_val;
    asm volatile("AT S1E0R, %0" : : "r" (vaddr));
    isb(); 
    asm volatile("MRS %0, PAR_EL1" : "=r" (par_val));
    if (par_val & 1) {
        return 0;
    }
    return (par_val & GENMASK_ULL(47, 12)) | (vaddr & 0xFFF);
}

static long handle_stealth_scan(struct stealth_scan_req* req, struct ShmData* shm) {
    struct AobScanRequest* scan_req = (struct AobScanRequest*)shm->cmd_data;
    uint64_t current_vaddr = scan_req->start_vaddr;
    uint64_t end_vaddr = current_vaddr + scan_req->scan_size;
    
    req->found_vaddr = 0;

    /* asm volatile(".inst 0xd503419f" ::: "memory"); */ /* msr tco, #1 (Requires FEAT_MTE, panics on older CPUs) */

    while (current_vaddr < end_vaddr) {
        uint64_t phys_addr;
        cond_resched();
        
        phys_addr = safe_translate_el0(current_vaddr);
        if (!phys_addr) {
            current_vaddr = (current_vaddr + PAGE_SIZE) & ~(PAGE_SIZE - 1);
            continue;
        }
        
        uint64_t scan_bytes = PAGE_SIZE - (current_vaddr & (PAGE_SIZE - 1));
        if (current_vaddr + scan_bytes > end_vaddr) {
            scan_bytes = end_vaddr - current_vaddr;
        }

        kernel_neon_begin();
        
        /* Bypass PAN (Privileged Access Never) by reading the physical page via the kernel's EL1 linear map! */
        uint64_t offset = neon_scan_16byte_pattern(
            (const uint8_t*)__va(phys_addr), 
            scan_bytes, 
            scan_req->pattern, 
            scan_req->mask,
            scan_req->pattern_len
        );
        
        kernel_neon_end();
        
        if (offset != 0) {
            req->found_vaddr = current_vaddr + offset;
            break;
        }
        
        current_vaddr += scan_bytes;
    }

    /* asm volatile(".inst 0xd503409f" ::: "memory"); */ /* msr tco, #0 */
    
    return 0;
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Senior Android Kernel Security Researcher");
MODULE_DESCRIPTION("Stealth LKM: Zero-Copy Fast IPC via Kretprobe Instance Hook");

/* ========================================================================
 * Kprobe Resolver
 * ======================================================================== */

typedef unsigned long (*kln_p)(const char *);
static kln_p kln_ptr = NULL;

static unsigned long resolve_via_kprobes(void)
{
	unsigned long addr = 0;
	struct kprobe kp = {
		.symbol_name = "kallsyms_lookup_name",
	};

	if (register_kprobe(&kp) < 0) {
		return 0;
	}

	addr = (unsigned long)kp.addr;
	unregister_kprobe(&kp);

	return addr;
}

static void *resolve_symbol(const char *name)
{
	if (!kln_ptr) {
		unsigned long addr = resolve_via_kprobes();
		if (!addr) return NULL;
		kln_ptr = (kln_p)addr;
	}

	return (void *)kln_ptr(name);
}

/* ========================================================================
 * GKI Symbols & Fast IPC Engine
 * ======================================================================== */

typedef struct pid *(*find_vpid_t)(int nr);
typedef struct task_struct *(*pid_task_t)(struct pid *pid, enum pid_type type);
typedef struct mm_struct *(*get_task_mm_t)(struct task_struct *task);
typedef void (*mmput_t)(struct mm_struct *mm);
typedef void (*kthread_use_mm_t)(struct mm_struct *mm);
typedef void (*kthread_unuse_mm_t)(struct mm_struct *mm);
typedef struct perf_event *(*register_user_hw_breakpoint_t)(struct perf_event_attr *attr, perf_overflow_handler_t triggered, void *context, struct task_struct *tsk);
typedef void (*unregister_hw_breakpoint_t)(struct perf_event *bp);
typedef int (*task_work_add_t)(struct task_struct *task, struct callback_head *twork, enum task_work_notify_mode mode);

static find_vpid_t gki_find_vpid = NULL;
static pid_task_t gki_pid_task = NULL;
static get_task_mm_t gki_get_task_mm = NULL;
static mmput_t gki_mmput = NULL;
static kthread_use_mm_t g_use_mm = NULL;
static kthread_unuse_mm_t g_unuse_mm = NULL;
static register_user_hw_breakpoint_t gki_register_user_hw_breakpoint = NULL;
static unregister_hw_breakpoint_t gki_unregister_hw_breakpoint = NULL;
static task_work_add_t gki_task_work_add = NULL;

void *g_shared_buffer = NULL;
EXPORT_SYMBOL(g_shared_buffer);
static struct task_struct *g_turbo_thread = NULL;
static struct page **g_user_pages = NULL;
static int g_num_pages = 0;

struct turbo_context {
	pid_t target_pid;
};

static struct turbo_context g_turbo_ctx = {0};

static inline uint32_t load_acquire_pc(volatile uint32_t *ptr) {
	uint32_t val;
	asm volatile(
		".arch_extension rcpc\n"
		"ldapr %w0, [%1]\n"
		: "=r" (val)
		: "r" (ptr)
		: "memory"
	);
	return val;
}

static inline uint64_t hw_translate_at_s1e0r(uint64_t vaddr) {
	uint64_t par_val;
	asm volatile(
		"at s1e0r, %1\n"
		"isb\n"
		"mrs %0, par_el1\n"
		: "=r" (par_val)
		: "r" (vaddr)
		: "memory"
	);
	if (par_val & 1) return 0;
	return (par_val & 0xFFFFFFFFF000ULL) | (vaddr & 0xFFFULL);
}

static int turbo_worker_thread(void *data) {
	struct ShmData *shm = (struct ShmData *)g_shared_buffer;
	struct mm_struct *active_mm = NULL;
	struct mm_struct *target_mm = NULL;
	bool mm_in_use = false;
	struct pid *ps = NULL;
	struct task_struct *t = NULL;

	pr_info("Stealth LKM: Turbo (Fast I/O) starting for PID: %d\n", g_turbo_ctx.target_pid);
	shm->magic_verify = 0xDEADC0DE; 

	if (gki_find_vpid && gki_pid_task && gki_get_task_mm) {
		ps = gki_find_vpid(g_turbo_ctx.target_pid);
		if (ps) {
			t = gki_pid_task(ps, PIDTYPE_PID);
			if (t) target_mm = gki_get_task_mm(t);
		}
	}

	while (!kthread_should_stop()) {
		uint32_t cur_seq = load_acquire_pc(&shm->user_seq);
		bool processed_command = false;

		if (cur_seq != shm->kernel_seq) {
			shm->turbo_counter++;

			if (active_mm != target_mm) {
				if (mm_in_use && g_unuse_mm) { g_unuse_mm(active_mm); mm_in_use = false; }
				active_mm = target_mm;
				if (active_mm && g_use_mm) { g_use_mm(active_mm); mm_in_use = true; }
			}

			if (shm->cmd_type == 1) {
				if (mm_in_use) shm->cmd_paddr_out = hw_translate_at_s1e0r(shm->cmd_vaddr);
				else shm->cmd_paddr_out = 0;
			}
			else if (shm->cmd_type == 5) {
				if (mm_in_use && shm->cmd_len > 0 && shm->cmd_len <= MAX_CMD_DATA_SIZE) {
					uint64_t current_va = shm->cmd_vaddr & 0x00FFFFFFFFFFFFFFULL;
					uint32_t remaining = shm->cmd_len;
					uint32_t copied = 0;

					while (remaining > 0) {
						uint64_t pa = hw_translate_at_s1e0r(current_va);
						if (pa) {
							uint32_t offset = pa & (PAGE_SIZE - 1);
							uint32_t chunk = PAGE_SIZE - offset;
							if (chunk > remaining) chunk = remaining;
							
							asm volatile(".inst 0xd503419f" ::: "memory"); /* msr tco, #1 */
							memcpy(shm->cmd_data + copied, __va(pa), chunk);
							asm volatile(".inst 0xd503409f" ::: "memory"); /* msr tco, #0 */
							
							copied += chunk;
							current_va += chunk;
							remaining -= chunk;
						} else {
							memset(shm->cmd_data + copied, 0, remaining);
							break;
						}
					}
					shm->cmd_paddr_out = copied;
				} else { 
					shm->cmd_paddr_out = 0; 
				}
			}

			if (shm->batch_active && mm_in_use) {
				uint32_t count = shm->batch_count;
				uint32_t i;
				if (count > MAX_BATCH_ENTRIES) count = MAX_BATCH_ENTRIES;

				for (i = 0; i < count; i++) {
					uint64_t va = shm->batch[i].vaddr;
					uint64_t untagged_va, pa;
					if (!va) continue;
					
					untagged_va = va & 0x00FFFFFFFFFFFFFFULL;
					pa = hw_translate_at_s1e0r(untagged_va);

					if (pa) {
						uint32_t read_sz = shm->batch[i].size;
						uint64_t val1 = 0, val2 = 0;
						bool success = true;
						uint64_t kernel_va;

						if (read_sz > 16) read_sz = 16;
						kernel_va = (uint64_t)__va(pa);
						
						/* MTE Bypass */
						asm volatile(".inst 0xd503419f" ::: "memory"); /* msr tco, #1 */
						val1 = *(volatile uint64_t *)kernel_va;
						
						if (read_sz > 8) {
							uint64_t pa2 = hw_translate_at_s1e0r(untagged_va + 8);
							if (pa2) val2 = *(volatile uint64_t *)__va(pa2);
							else success = false;
						}
						asm volatile(".inst 0xd503409f" ::: "memory"); /* msr tco, #0 */
						
						if (success) {
							shm->batch[i].status = 1;
							*(uint64_t*)(shm->batch[i].data) = val1;
							if (read_sz > 8) *(uint64_t*)(shm->batch[i].data + 8) = val2;
						} else {
							shm->batch[i].status = 0;
						}
					} else {
						shm->batch[i].status = 0;
					}
				}
				
				if (shm->batch_delay_ns > 0) ndelay(shm->batch_delay_ns);
			} 

			smp_store_release(&shm->kernel_seq, cur_seq);
			processed_command = true;
		}
		
		if (!processed_command) {
			/* Check SPE commands outside of seq-gated block */
			uint32_t spe_cmd = __atomic_load_n(&shm->cmd_type, __ATOMIC_ACQUIRE);
			if (spe_cmd == 8 /* CMD_SET_SPE_WATCHPOINT */) {
				uint64_t current_va = shm->cmd_vaddr & 0x00FFFFFFFFFFFFFFULL;
				set_spe_watchpoint(current_va, shm->cmd_len, g_turbo_ctx.target_pid);
				__atomic_store_n(&shm->cmd_type, 0, __ATOMIC_RELEASE);
			}
			else if (spe_cmd == 9 /* CMD_REMOVE_SPE_WATCHPOINT */) {
				stop_spe_engine();
				__atomic_store_n(&shm->cmd_type, 0, __ATOMIC_RELEASE);
			}
			else if (spe_cmd == 10 /* CMD_POLL_SPE_EVENTS */) {
				__atomic_store_n(&shm->cmd_type, 0, __ATOMIC_RELEASE);
			}
		}

		if (!processed_command) {
			int spin;
			for (spin = 0; spin < 50000; spin++) {
				if (load_acquire_pc(&shm->user_seq) != shm->kernel_seq) break;
				cpu_relax();
			}
			if (load_acquire_pc(&shm->user_seq) == shm->kernel_seq) {
				usleep_range(50, 100);
			}
		}
	}

	if (mm_in_use && g_unuse_mm) g_unuse_mm(active_mm);
	if (target_mm && gki_mmput) gki_mmput(target_mm);
	
	pr_info("Stealth LKM: Turbo worker stopping\n");
	return 0;
}

static void fast_ipc_stop_turbo(void)
{
	if (g_turbo_thread) {
		kthread_stop(g_turbo_thread);
		g_turbo_thread = NULL;
	}
	if (g_shared_buffer) {
		vunmap(g_shared_buffer);
		g_shared_buffer = NULL;
	}
	if (g_user_pages) {
		int i;
		for (i = 0; i < g_num_pages; i++) put_page(g_user_pages[i]);
		kfree(g_user_pages);
		g_user_pages = NULL;
		g_num_pages = 0;
	}
}

static int fast_ipc_start_turbo(pid_t target_pid, uint64_t user_vaddr)
{
	int ret, i;
	int num_pages = (PAGE_ALIGN(sizeof(struct ShmData))) / PAGE_SIZE;
	
	if (g_turbo_thread) return 0;
	
	g_turbo_ctx.target_pid = target_pid;
	
	g_user_pages = kmalloc_array(num_pages, sizeof(struct page *), GFP_KERNEL);
	if (!g_user_pages) return -ENOMEM;
	
	ret = get_user_pages_fast(user_vaddr, num_pages, FOLL_WRITE, g_user_pages);
	if (ret != num_pages) {
		if (ret > 0) { for (i = 0; i < ret; i++) put_page(g_user_pages[i]); }
		kfree(g_user_pages);
		g_user_pages = NULL;
		return -EFAULT;
	}
	g_num_pages = num_pages;
	
	g_shared_buffer = vmap(g_user_pages, num_pages, VM_MAP, PAGE_KERNEL);
	if (!g_shared_buffer) {
		for (i = 0; i < num_pages; i++) put_page(g_user_pages[i]);
		kfree(g_user_pages);
		g_user_pages = NULL;
		return -ENOMEM;
	}
	
	g_turbo_thread = kthread_run(turbo_worker_thread, NULL, "poly_turbo");
	if (IS_ERR(g_turbo_thread)) {
		int err = PTR_ERR(g_turbo_thread);
		g_turbo_thread = NULL;
		vunmap(g_shared_buffer);
		g_shared_buffer = NULL;
		for (i = 0; i < num_pages; i++) put_page(g_user_pages[i]);
		kfree(g_user_pages);
		g_user_pages = NULL;
		return err;
	}
	return 0;
}

/* ========================================================================
 * Hardware Breakpoint (HWBP) Engine
 * ======================================================================== */

#define MAX_HWBP_EVENTS 1024
static struct perf_event *g_hwbp_events[MAX_HWBP_EVENTS];
static int g_hwbp_count = 0;
static DECLARE_WAIT_QUEUE_HEAD(stealth_wait_queue);

/*
static void lkm_freeze_callback(struct callback_head *work)
{
	// Temporarily disabled for raw throughput testing
}
*/

static void hwbp_handler(struct perf_event *bp, struct perf_sample_data *data, struct pt_regs *regs)
{
	struct ShmData *shm = (struct ShmData *)g_shared_buffer;

	if (!shm) return;

	/* Ensure we only ever freeze on User-Mode Exceptions! */
	if (!user_mode(regs)) return;

	if (cmpxchg(&shm->debug_lock, 0, 1) != 0) {
		return;
	}

	/* CAPTURE EXACT STATE IMMEDAITELY ON EXCEPTION CONTEXT */
	memcpy(shm->debug_regs.regs, regs->regs, sizeof(uint64_t) * 31);
	shm->debug_regs.sp = regs->sp;
	shm->debug_regs.pc = regs->pc;
	shm->debug_regs.pstate = regs->pstate;

	/* RAW KERNEL DUMP TO PROVE WHAT THE EXCEPTION FRAME HOLDS */
	pr_err("Stealth HWBP [RAW]: PC=%016llx SP=%016llx PSTATE=%08llx\n", regs->pc, regs->sp, regs->pstate);
	pr_err("Stealth HWBP [RAW]: X0=%016llx X1=%016llx X2=%016llx\n", regs->regs[0], regs->regs[1], regs->regs[2]);

	/* Signal the event to userspace (non-blocking — we are in NMI/perf context!) */
	shm->debug_resume_flag = 0;
	smp_wmb();
	shm->debug_event_active = 1;

	/*
	 * IMPORTANT: We CANNOT sleep here (wait_event) because perf overflow
	 * handlers run in NMI/IRQ context. Instead, we just snapshot the regs
	 * and return. The userspace daemon polls debug_event_active, modifies
	 * debug_regs, sets debug_resume_flag=1, and calls RESUME_HWBP ioctl.
	 * The ioctl handler does NOT need to wake anything — the modification
	 * is picked up on the next breakpoint hit or via the daemon's own flow.
	 *
	 * For true thread freeze + register writeback, we would need task_work_add
	 * to defer into the target's own task context. For now, snapshot-only mode
	 * is sufficient for the SPE + HWBP demonstration.
	 */

	shm->debug_lock = 0;
}

static void remove_hwbp(void);

static int set_hwbp(int target_pid, uint64_t vaddr)
{
	struct perf_event_attr attr;
	struct pid *ps = NULL;
	struct task_struct *leader_task = NULL;
	struct task_struct *t;
	int *tid_array;
	int tid_count = 0;
	int i, ret = 0;

	if (!gki_find_vpid || !gki_pid_task) return -ENOENT;

	tid_array = kmalloc_array(MAX_HWBP_EVENTS, sizeof(int), GFP_KERNEL);
	if (!tid_array) return -ENOMEM;

	ps = gki_find_vpid(target_pid);
	if (!ps) { ret = -ESRCH; goto out; }
	
	leader_task = gki_pid_task(ps, PIDTYPE_PID);
	if (!leader_task) { ret = -ESRCH; goto out; }

	/* Pass 1: RCU Lockless PID Gathering */
	rcu_read_lock();
	for_each_thread(leader_task, t) {
		if (tid_count < MAX_HWBP_EVENTS) {
			tid_array[tid_count++] = t->pid; /* t->pid contains the TID inside the kernel */
		} else {
			break;
		}
	}
	rcu_read_unlock();

	if (tid_count == 0) { ret = -ESRCH; goto out; }

	hw_breakpoint_init(&attr);
	attr.bp_addr = vaddr;
	attr.bp_len = HW_BREAKPOINT_LEN_4; /* AArch64 instructions are 4 bytes */
	attr.bp_type = HW_BREAKPOINT_X;    /* Execute breakpoint */

	/* Clear previous state if unhandled */
	remove_hwbp();

	/* Pass 2: Register Hardware Breakpoints individually per resolved Task Struct */
	pr_info("Stealth LKM: Preparing to hook %d threads for TGID %d...\n", tid_count, target_pid);

	for (i = 0; i < tid_count; i++) {
		struct pid *t_pid;
		struct task_struct *t_task;
		struct perf_event *bp;

		t_pid = gki_find_vpid(tid_array[i]);
		if (!t_pid) continue;
		
		t_task = gki_pid_task(t_pid, PIDTYPE_PID);
		if (!t_task) continue;

		bp = gki_register_user_hw_breakpoint(&attr, hwbp_handler, NULL, t_task);
		if (IS_ERR(bp)) {
			/* If we exhaust debug registers (usually 4-6 hardware slots total across cores),
			 * the kernel scheduler will internally queue these perf_events and multiplex them.
			 * We still track them so they can be cleaned up later. */
			pr_warn("Stealth LKM: register_user_hw_breakpoint failed for TID %d (err: %ld)\n", tid_array[i], PTR_ERR(bp));
		} else {
			g_hwbp_events[g_hwbp_count++] = bp;
		}
	}

	pr_info("Stealth LKM: Successfully planted %d active HWBPs at 0x%llx\n", g_hwbp_count, vaddr);

out:
	kfree(tid_array);
	return ret;
}

static void remove_hwbp(void)
{
	int i;
	for (i = 0; i < g_hwbp_count; i++) {
		if (g_hwbp_events[i]) {
			gki_unregister_hw_breakpoint(g_hwbp_events[i]);
			g_hwbp_events[i] = NULL;
		}
	}
	if (g_hwbp_count > 0) {
		pr_info("Stealth LKM: Removed %d HWBPs.\n", g_hwbp_count);
	}
	g_hwbp_count = 0;
}

/* ========================================================================
 * ASHMEM VFS Hook (via ashmem_open Kretprobe)
 *
 * Безопасно парсит структуру ioctl, т.к. мы находимся в оригинальном,
 * обычном контексте VFS (разрешающем `copy_from_user` и сон). Мы хукаем
 * возврат из `ashmem_open` и меняем `file->f_op` только для конкретного `fd`.
 * ======================================================================== */

static const struct file_operations *g_orig_ashmem_fops = NULL;
static struct file_operations g_stealth_fops;

static long stealth_ashmem_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	if (cmd == STEALTH_IOCTL_PING) {
		pr_info("Stealth LKM: PING via Instance VFS Hook!\n");
		return 0; // Success
	} 
	else if (cmd == STEALTH_IOCTL_START_TURBO) {
		struct stealth_turbo_init init_data;
		if (copy_from_user(&init_data, (void __user *)arg, sizeof(init_data))) {
			return -EFAULT;
		}
		pr_info("Stealth LKM: Starting Turbo IPC for PID %d at 0x%llx\n", 
			init_data.target_pid, init_data.shared_vaddr);
		return fast_ipc_start_turbo(init_data.target_pid, init_data.shared_vaddr);
	}
	else if (cmd == STEALTH_IOCTL_STOP_TURBO) {
		fast_ipc_stop_turbo();
		return 0;
	}
	else if (cmd == STEALTH_IOCTL_SET_HWBP) {
		struct stealth_hwbp_req req;
		if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
			return -EFAULT;
		return set_hwbp(req.target_pid, req.vaddr);
	}
	else if (cmd == STEALTH_IOCTL_REMOVE_HWBP) {
		remove_hwbp();
		return 0;
	}
	else if (cmd == STEALTH_IOCTL_RESUME_HWBP) {
		wake_up_interruptible(&stealth_wait_queue);
		return 0;
	}
	else if (cmd == STEALTH_IOCTL_SCAN_PATTERN) {
		struct stealth_scan_req req;
		if (!g_shared_buffer) return -EINVAL;
		if (copy_from_user(&req, (void __user *)arg, sizeof(req)))
			return -EFAULT;
		
		handle_stealth_scan(&req, (struct ShmData *)g_shared_buffer);
		
		if (copy_to_user((void __user *)arg, &req, sizeof(req)))
			return -EFAULT;
		return 0;
	}

	/* Если команда не наша, проксируем вызов в оригинальный драйвер */
	if (g_orig_ashmem_fops && g_orig_ashmem_fops->unlocked_ioctl) {
		return g_orig_ashmem_fops->unlocked_ioctl(file, cmd, arg);
	}

	return -ENOTTY;
}

static struct kretprobe ashmem_open_kretprobe;

static int ashmem_open_entry(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	/* AArch64 ABI: arg 2 is in x1 / regs->regs[1] (struct file *file) */
	struct file *file = (struct file *)regs->regs[1];
	*((struct file **)ri->data) = file;
	return 0;
}

static int ashmem_open_ret(struct kretprobe_instance *ri, struct pt_regs *regs)
{
	int ret = regs->regs[0]; /* x0 has the return code */
	struct file *file = *((struct file **)ri->data);

	/* If open succeeded, modify the f_op for this specific file instance */
	if (ret == 0 && file && file->f_op) {
		if (!g_orig_ashmem_fops) {
			g_orig_ashmem_fops = file->f_op;
			memcpy(&g_stealth_fops, g_orig_ashmem_fops, sizeof(g_stealth_fops));
			g_stealth_fops.unlocked_ioctl = stealth_ashmem_ioctl;
		}
		/* Overwrite the file operations pointer for this dynamically allocated `struct file` */
		file->f_op = &g_stealth_fops;
	}
	return 0;
}

static int __init stealth_analyzer_init(void)
{
	int ret;

	pr_info("Stealth LKM: Initializing...\n");

	gki_find_vpid = (find_vpid_t)resolve_symbol("find_vpid");
	gki_pid_task = (pid_task_t)resolve_symbol("pid_task");
	gki_get_task_mm = (get_task_mm_t)resolve_symbol("get_task_mm");
	gki_mmput = (mmput_t)resolve_symbol("mmput");
	g_use_mm = (kthread_use_mm_t)resolve_symbol("kthread_use_mm");
	g_unuse_mm = (kthread_unuse_mm_t)resolve_symbol("kthread_unuse_mm");
	gki_register_user_hw_breakpoint = (register_user_hw_breakpoint_t)resolve_symbol("register_user_hw_breakpoint");
	gki_unregister_hw_breakpoint = (unregister_hw_breakpoint_t)resolve_symbol("unregister_hw_breakpoint");
	gki_task_work_add = (task_work_add_t)resolve_symbol("task_work_add");

	if (!gki_find_vpid || !gki_mmput || !gki_register_user_hw_breakpoint || !gki_unregister_hw_breakpoint || !gki_task_work_add) {
		pr_err("Stealth LKM: Failed to resolve essential GKI symbols.\n");
		return -ENOENT;
	}

	/* Setup Kretprobe on ashmem_open */
	ashmem_open_kretprobe.kp.symbol_name = "ashmem_open";
	ashmem_open_kretprobe.entry_handler = ashmem_open_entry;
	ashmem_open_kretprobe.handler = ashmem_open_ret;
	ashmem_open_kretprobe.data_size = sizeof(struct file *);
	ashmem_open_kretprobe.maxactive = 64;

	ret = register_kretprobe(&ashmem_open_kretprobe);
	if (ret < 0) {
		pr_err("Stealth LKM: register_kretprobe(ashmem_open) failed, returned %d\n", ret);
		return ret;
	}

	pr_info("Stealth LKM: Kretprobe Hook on ashmem_open installed successfully!\n");

	return 0;
}

static void __exit stealth_analyzer_exit(void)
{
	cleanup_spe_engine();
	fast_ipc_stop_turbo();
	remove_hwbp();
	unregister_kretprobe(&ashmem_open_kretprobe);
	pr_info("Stealth LKM: Unloaded. Kretprobe removed.\n");
}

module_init(stealth_analyzer_init);
module_exit(stealth_analyzer_exit);
