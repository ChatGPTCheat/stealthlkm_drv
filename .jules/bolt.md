## 2024-05-18 - format! in egui Immediate Mode UI
**Learning:** Using `format!` within `egui` rendering loops causes massive overhead due to continuous per-iteration string heap allocations (60 times a second).
**Action:** Use `std::fmt::Write` and the `write!` macro directly on pre-allocated strings (`String::with_capacity`) inside eframe/egui render loops instead of `push_str(&format!(...))`.
