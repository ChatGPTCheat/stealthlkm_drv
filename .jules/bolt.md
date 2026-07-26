## 2024-10-24 - eframe/egui UI Rendering Loop Format Macro Optimization
**Learning:** In eframe/egui immediate-mode UI rendering loops, using `format!` causes massive overhead due to continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and the `write!` macro directly on pre-allocated strings (e.g., `String::with_capacity`) instead of `format!` within UI rendering loops.
