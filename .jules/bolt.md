## 2024-10-24 - Avoid format! inside egui render loops
**Learning:** Using `format!` within immediate-mode UI rendering loops incurs massive overhead due to continuous per-iteration string heap allocations.
**Action:** Pre-allocate `String::with_capacity` and use `std::fmt::Write` macros (`write!`) directly on the buffer to avoid redundant allocations.
