## 2024-05-24 - String Allocation in Render Loop
**Learning:** Using `format!` within UI rendering loops (like hex dump generation) causes thousands of allocations per second, drastically increasing GC/drop overhead and impacting frame rates in eframe applications.
**Action:** Always pre-allocate strings and use `std::fmt::Write` (`write!`) directly on the buffer to avoid per-iteration allocations. Use import aliasing when both `io::Write` and `fmt::Write` are needed.
