## 2026-07-14 - Optimize string formatting in loops
**Learning:** Using `format!` inside loops causes severe performance degradation due to repeated string allocations.
**Action:** Use `std::fmt::Write` and the `write!` macro on pre-allocated strings instead.
