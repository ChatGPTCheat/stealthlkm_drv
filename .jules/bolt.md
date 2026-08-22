## 2024-08-22 - Avoid format! in eframe/egui render loops
**Learning:** Continuous per-iteration string heap allocations using `format!` inside immediate-mode UI rendering loops (like egui) incur massive overhead, severely degrading frame times, especially in data-heavy views like hex dumps.
**Action:** Always use `std::fmt::Write` and the `write!` macro directly on pre-allocated strings (`String::with_capacity`) inside hot UI rendering paths to minimize heap allocations.
