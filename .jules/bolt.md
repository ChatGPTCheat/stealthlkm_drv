## 2023-10-25 - Immediate-Mode UI String Allocation Bottleneck
**Learning:** Building multi-element strings (like hex dumps) using `format!` inside `egui` rendering loops causes massive overhead due to continuous, per-iteration string heap allocations at 60 FPS.
**Action:** Use `std::fmt::Write` and `write!` on pre-allocated strings (`String::with_capacity`) for loops building multi-element strings in the UI path. For single-line UI labels, `format!` remains acceptable.
