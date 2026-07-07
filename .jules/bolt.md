## 2024-05-24 - Avoid `format!` inside hot loops in Rust UI components
**Learning:** Using `format!` within tight loops (like hex dump generation or GDB server memory mapping) causes massive performance degradation due to unnecessary per-iteration string allocations.
**Action:** Always pre-allocate strings with `String::with_capacity()` and use `std::fmt::Write` via the `write!` macro for in-place string mutation to prevent allocation overhead.
