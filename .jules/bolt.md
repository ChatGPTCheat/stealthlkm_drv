## 2026-08-06 - Optimize string formatting in egui loops
**Learning:** Using `format!` inside immediate-mode UI rendering loops incurs massive overhead due to continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and the `write!` macro directly on pre-allocated strings (`String::with_capacity`) instead.
