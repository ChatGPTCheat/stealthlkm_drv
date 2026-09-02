## 2024-05-24 - Avoiding format! in egui rendering loops
**Learning:** In the Rust GUI application (`gui/`), using `format!` inside eframe/egui immediate-mode UI rendering loops incurs massive overhead due to continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and the `write!` macro directly on pre-allocated strings (`String::with_capacity`) instead of `format!` inside loops.
