## 2025-02-12 - Prevent format! inside loops in egui
**Learning:** Using `format!` inside hot loops (like egui immediate mode rendering loops for hex viewer or disassembly dumps) causes massive performance overhead from continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and `write!` on pre-allocated strings (e.g. `String::with_capacity` or just writing to a reusable buffer) instead of continuous `format!` in loops.
