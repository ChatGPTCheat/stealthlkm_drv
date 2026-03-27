# Stealth AArch64 Kernel Memory Analysis Framework 🕵️‍♂️🏎️

A high-performance, Ring-0 memory mapping and real-time debugging framework for Android GKI 6.1 (AArch64). Built to entirely bypass heavy Userspace Anti-Cheat mechanisms by relying on in-kernel hardware breakpoint control and physical-to-virtual memory mapping.

## Architecture

The system is separated into three highly optimized tiers that communicate with complete synchrony and near-zero latency:

### 1. The Kernel Module (`kernel/`)
- Utilizes `Kretprobes` on standard `ashmem_open` to inject a suite of custom `file_operations` into `/dev/ashmem` invisibly.
- Instantiates a dedicated kernel thread (`poly_turbo`) polling a shared memory buffer `ShmData` ring buffer.
- Performs hardware-level page table translation (`hw_translate_at_s1e0r`) directly bypassing standard `copy_from_user` overhead in the kernel.
- Achieves incredible **~170ns read speeds** for dynamic memory batches.
- Harnesses `register_user_hw_breakpoint` coupled with `task_work` events to secretly freeze game threads upon hardware watchpoint triggers.

### 2. The Userspace Daemon (`daemon/`)
- A C++ background server opening a TCP Socket bridging the Ring-0 engine with any external tooling.
- Uses `mmap` zero-copy IPC structures to push thousands of batch structures concurrently from `stealth_daemon.cpp` to the kernel driver.
- Parses process mappings accurately via direct `map_parser.cpp` resolving ELF base addresses instantly.

### 3. The Rust GUI & GDB Server (`gui/`)
- A stunning Rust `egui`-based control panel.
- Interprets dynamic raw memory streams live, formats Hex streams, extracts Pointers and handles `f32` vectors.
- Includes a live hardware `Capstone` AArch64 Disassembly view.
- Embeds a powerful **GDB Remote Serial Protocol (RSP)** server proxy (`23946`) tricking standard **IDA Pro** into acting as a pure dynamic remote debugger. Complete integration with native ARM64 IDA templates, displaying realtime PC and registers without needing ptrace!

## Building

### Daemon
Compile using the Android NDK `aarch64-linux-android-clang++`:
```bash
cd daemon
# Assuming NDK variables are set:
make
```

### Kernel Module
Use the standard GKI compilation tree:
```bash
cd kernel
make all
```

### GUI Analyzer
Ensure a standard Rust toolchain is installed:
```bash
cd gui
cargo run --release
```

## Disclaimer
Created strictly for educational and performance analysis research.
