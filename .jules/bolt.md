## 2023-10-29 - [Optimization: Avoid format! in loops]
**Learning:** In Rust GUI rendering loops, using `format!` continuously to construct multi-element strings (e.g., hex bytes per chunk in the immediate mode UI layout, or disassembly logs) causes massive overhead from continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` via the `write!` macro onto pre-allocated `String::with_capacity` buffers instead of `format!`.
