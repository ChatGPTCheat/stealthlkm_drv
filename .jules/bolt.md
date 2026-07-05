## 2024-05-15 - Optimize string formatting loop
**Learning:** Using `format!` within a loop incurs significant overhead due to per-iteration string allocations.
**Action:** Use `std::fmt::Write` and `write!` macro directly on pre-allocated strings to avoid unnecessary allocations.
