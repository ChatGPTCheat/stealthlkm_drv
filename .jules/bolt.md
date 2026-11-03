## 2024-03-24 - Avoid string formatting in hot paths
**Learning:** `format!` inside `egui` immediate-mode UI rendering loops incurs massive overhead from continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and `write!` on pre-allocated strings (`String::with_capacity`) instead, or `push_str` with pre-computed buffers.

## 2024-03-24 - Cache Capstone initialization
**Learning:** Initializing the `Capstone` disassembly engine for AArch64 is computationally expensive.
**Action:** Avoid re-initializing it per call (e.g., in `disassemble_arm64`); instead cache the instance in the global application state.
