## 2024-10-12 - Optimize String Allocation in UI Render Loop
**Learning:** Avoid using `format!` and `.push_str` repeatedly inside immediate-mode GUI loops like `egui` (especially for hex and assembly output grids). It incurs huge overhead due to continuous heap allocations per frame per chunk.
**Action:** Use `std::fmt::Write` with pre-allocated strings (`String::with_capacity`) and the `write!` macro instead for these UI elements.
