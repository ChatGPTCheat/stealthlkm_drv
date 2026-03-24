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
#include <linux/ktime.h>
#include <linux/task_work.h>
#include <linux/wait.h>

#include "arch_arm64.h"
#include "shared_buffer.h"

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

static void *g_shared_buffer = NULL;
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
					uint64_t pa = hw_translate_at_s1e0r(shm->cmd_vaddr);
					if (pa) {
						shm->cmd_paddr_out = pa;
						memset(shm->cmd_data, 0, shm->cmd_len); 
					} else {
						shm->cmd_paddr_out = 0;
						memset(shm->cmd_data, 0, shm->cmd_len);
					}
				} else { shm->cmd_paddr_out = 0; }
			}

			smp_store_release(&shm->kernel_seq, cur_seq);
			processed_command = true;
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
			
			shm->turbo_counter++;
			processed_command = true;
			
			if (shm->batch_delay_ns > 0) ndelay(shm->batch_delay_ns);
			else cond_resched();
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

static struct perf_event *g_hwbp_event = NULL;
static DECLARE_WAIT_QUEUE_HEAD(stealth_wait_queue);

static void lkm_freeze_callback(struct callback_head *work)
{
	struct pt_regs *regs;
	struct ShmData *shm = (struct ShmData *)g_shared_buffer;

	if (!shm) return;

	regs = task_pt_regs(current);
	if (!regs) {
		shm->debug_lock = 0;
		return;
	}

	/* Copy to SHM */
	memcpy(shm->debug_regs.regs, regs->regs, sizeof(uint64_t) * 31);
	shm->debug_regs.sp = regs->sp;
	shm->debug_regs.pc = regs->pc;
	shm->debug_regs.pstate = regs->pstate;

	shm->debug_resume_flag = 0;
	smp_wmb();
	shm->debug_event_active = 1;

	/* Safe Sleep via wait_queue */
	for (;;) {
		DEFINE_WAIT(wait);
		prepare_to_wait(&stealth_wait_queue, &wait, TASK_INTERRUPTIBLE);

		if (shm->debug_resume_flag) {
			finish_wait(&stealth_wait_queue, &wait);
			break;
		}

		schedule();
		finish_wait(&stealth_wait_queue, &wait);
	}

	/* Copy back from SHM */
	memcpy(regs->regs, shm->debug_regs.regs, sizeof(uint64_t) * 31);
	regs->sp = shm->debug_regs.sp;
	regs->pc = shm->debug_regs.pc;
	regs->pstate = shm->debug_regs.pstate; /* Даем читу полный контроль над PSTATE (в т.ч. флагом SS) */

	shm->debug_event_active = 0;
	smp_wmb();
	shm->debug_lock = 0;
}

static void hwbp_handler(struct perf_event *bp, struct perf_sample_data *data, struct pt_regs *regs)
{
	struct ShmData *shm = (struct ShmData *)g_shared_buffer;
	static struct callback_head stealth_tw;

	if (!shm) return;

	if (cmpxchg(&shm->debug_lock, 0, 1) != 0) {
		return;
	}

	if (gki_task_work_add) {
		init_task_work(&stealth_tw, lkm_freeze_callback);
		/* TWA_RESUME безопаснее, так как не отправляет процессу фейковый сигнал (TIF_SIGPENDING),
		 * который ломает Single-Step обработчик внутри do_notify_resume. */
		gki_task_work_add(current, &stealth_tw, TWA_RESUME);
	} else {
		shm->debug_lock = 0;
	}
}

static int set_hwbp(int target_pid, uint64_t vaddr)
{
	struct perf_event_attr attr;
	struct pid *ps = NULL;
	struct task_struct *t = NULL;

	if (!gki_find_vpid || !gki_pid_task) return -ENOENT;

	ps = gki_find_vpid(target_pid);
	if (!ps) return -ESRCH;
	
	t = gki_pid_task(ps, PIDTYPE_PID);
	if (!t) return -ESRCH;

	hw_breakpoint_init(&attr);
	attr.bp_addr = vaddr;
	attr.bp_len = HW_BREAKPOINT_LEN_4; /* AArch64 instructions are 4 bytes */
	attr.bp_type = HW_BREAKPOINT_X;    /* Execute breakpoint */

	/* CRITICAL FIX: User addresses MUST use register_user_hw_breakpoint attached to a specific task */
	g_hwbp_event = gki_register_user_hw_breakpoint(&attr, hwbp_handler, NULL, t);
	if (IS_ERR(g_hwbp_event)) {
		int err = PTR_ERR(g_hwbp_event);
		g_hwbp_event = NULL;
		pr_err("Stealth LKM: Failed to register HWBP for PID %d at 0x%llx (err: %d)\n", target_pid, vaddr, err);
		return err;
	}

	pr_info("Stealth LKM: HWBP successfully planted for PID %d at 0x%llx\n", target_pid, vaddr);
	return 0;
}

static void remove_hwbp(void)
{
	if (g_hwbp_event) {
		gki_unregister_hw_breakpoint(g_hwbp_event);
		g_hwbp_event = NULL;
		pr_info("Stealth LKM: HWBP removed.\n");
	}
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
	fast_ipc_stop_turbo();
	remove_hwbp();
	unregister_kretprobe(&ashmem_open_kretprobe);
	pr_info("Stealth LKM: Unloaded. Kretprobe removed.\n");
}

module_init(stealth_analyzer_init);
module_exit(stealth_analyzer_exit);
