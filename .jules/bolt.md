## 2024-05-24 - eframe UI String Allocations
**Learning:** Using `format!` inside `egui` rendering loops causes severe performance degradation due to rapid heap allocations every frame (e.g., thousands of string constructions per second in memory viewer loops).
**Action:** Use `std::fmt::Write` via the `write!` macro on strings pre-allocated with `String::with_capacity` to update buffers in-place safely without trailing heap churn.
