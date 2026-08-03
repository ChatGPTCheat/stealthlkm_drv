## 2024-05-24 - Performance string allocation
**Learning:** Usage of `format!` inside loops causes massive overhead due to continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and `write!` macro directly on pre-allocated strings (`String::with_capacity`). In files that also import `std::io::Write`, use import aliasing to prevent naming collisions.
