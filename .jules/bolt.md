## 2024-05-18 - [Optimizing String Formatting in Immediate-Mode GUIs]
**Learning:** Using `format!` inside `egui` render loops causes severe performance degradation due to continuous per-iteration string heap allocations every frame.
**Action:** Replace `format!` with `std::fmt::Write` (using `write!`) on pre-allocated strings (`String::with_capacity`) inside any code that executes per-frame.
