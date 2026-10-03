## 2024-10-24 - Optimizing egui render loops
**Learning:** In egui immediate-mode UI frameworks, code runs every single frame. Using `format!` inside tight loops like memory hex dump rendering creates massive continuous heap allocations, drastically reducing performance.
**Action:** Always use `std::fmt::Write` with pre-allocated strings (`String::with_capacity`) and the `write!` macro for string building inside hot render paths.
