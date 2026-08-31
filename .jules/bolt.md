## 2023-11-20 - [Avoid string formatting in eframe rendering loops]
**Learning:** [In Rust components, especially within eframe/egui immediate-mode UI rendering loops that execute every frame, avoid using `format!` within loops. It incurs massive overhead due to continuous per-iteration string heap allocations.]
**Action:** [Use `std::fmt::Write` and the `write!` macro directly on pre-allocated strings (`String::with_capacity`) instead.]
