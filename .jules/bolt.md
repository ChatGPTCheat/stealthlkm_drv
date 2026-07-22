## 2025-01-20 - Avoid format! in eframe/egui loops
**Learning:** Using `format!` within eframe/egui immediate-mode UI rendering loops that execute every frame incurs massive overhead due to continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and the `write!` macro directly on pre-allocated strings (e.g., `String::with_capacity`) instead of `format!` and `push_str` in rendering and data serialization loops.
