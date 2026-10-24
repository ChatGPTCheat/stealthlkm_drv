## 2024-05-14 - [Optimize Hex Viewer Rendering]
**Learning:** Using `format!` inside eframe/egui immediate-mode UI rendering loops incurs massive overhead from continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and `write!` on pre-allocated strings (`String::with_capacity`) instead for hot paths, reserving `format!` for single-line UI labels where `egui` requires cloning anyway.
