## 2024-09-26 - [Avoid format! in egui hot rendering loop]
**Learning:** Using `format!` within an egui immediate-mode UI rendering loop (which executes every frame) incurs massive overhead due to continuous per-iteration string heap allocations.
**Action:** Instead of `format!`, use `std::fmt::Write` and the `write!` macro directly on pre-allocated strings (`String::with_capacity`) in hot rendering paths.
