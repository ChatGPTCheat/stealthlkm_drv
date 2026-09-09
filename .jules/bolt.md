## 2024-05-13 - String Formatting Loop Optimization in egui
**Learning:** In Rust eframe/egui apps, `format!` within immediate-mode rendering loops incurs massive overhead due to continuous heap allocations every frame. This applies to rendering lists, hex grids, and formatting strings for data viewing.
**Action:** Use `std::fmt::Write` (e.g. `write!`) directly on pre-allocated strings (`String::with_capacity`) inside loops instead of string concatenation with `format!`.
