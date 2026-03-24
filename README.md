# Stealth LKM Memory Engine (Android GKI)

A high-performance, stealthy Ring-0 Memory Analysis & Debugging framework for Android AArch64 (GKI 6.1). Designed for dynamic game analysis, bypassing modern Anti-Cheats (e.g. Tencent, Epic Games, XignCode) without using `ptrace`, `/proc/pid/mem`, or `sys_process_vm_readv`.

## Features
* **Zero-Copy IPC**: Bridges userspace and kernel via a shared memory buffer (`mmap`), offering sub-microsecond memory reading speeds (~170ns per read) without any system calls.
* **Invisible VFS Hooking**: Injects a proxy `file_operations` hook dynamically into `/dev/ashmem` via `Kretprobe`, leaving standard sysfs and `kallsyms` un-tampered.
* **MTE & PAC Bypasses**: Disables ARM Memory Tagging Extension checking temporarily (`msr tco, #1`) allowing safe reads on modern Pixel and Snapdragon 8 Gen 2+ devices.
* **Hardware Breakpoints (HWBP) Freezing**: Features an advanced `task_work` + `wait_queue` freezing mechanism that traps target threads (e.g. RenderThread) safely inside EL1 without triggering Kernel Watchdogs.
* **Early Return PC-Patching**: Effortlessly skip function execution (Anti-Cheat initializations, SSL encryptions) manually from a C++ client by overwriting `X30` (Link Register) into `PC`.

## Project Structure
- `stealth_analyzer.c`: The core Rootkit/LKM.
- `arch_arm64.h`: AArch64 specific assembly inline primitives.
- `shared_buffer.h`: Shared C structures and IOCTL macros for bidirectional communication.
- `memory_engine.h/cpp`: The C++ wrapper class abstracting IPC, Batched Memory reading and HWBP Debugging API.
- `main.cpp`: A fully functional example of injecting into an Android game, setting HW breakpoints, and hot-patching CPU registers.

## Build Requirements
- Android NDK (Clang >= 14)
- AArch64 Linux Toolchain
- GKI 6.1 Kernel Source headers (for building the module)

## Usage
1. Build the kernel module:
   ```bash
   make ARCH=arm64 LLVM=1
   ```
2. Build the C++ client:
   ```bash
   clang++ -std=c++17 -O3 -Wall -pthread -I. -o engine_tester memory_engine.cpp main.cpp
   ```
3. Push to an adb-rooted device:
   ```bash
   adb push stealth_analyzer.ko /data/local/tmp/
   adb push engine_tester /data/local/tmp/
   ```
4. Run:
   ```bash
   insmod /data/local/tmp/stealth_analyzer.ko
   /data/local/tmp/engine_tester self
   ```

## Disclaimer
This project is strictly for Educational & Kernel Security Research purposes. It demonstrates severe vulnerabilities in Android's EL1/EL0 boundary controls and `perf_event` debugging. The author assumes no responsibility for abusive applications of this framework.
