## 2024-07-12 - Inefficient string concatenation in loops using format!
**Learning:** Using `format!` within tight loops (like iterating over memory buffers to generate hex strings) is a significant performance bottleneck due to continuous and unnecessary string re-allocations per iteration.
**Action:** Replace `format!` in loops with `std::fmt::Write` (e.g., `write!(str_buf, ...).unwrap()`) onto pre-allocated `String` buffers to drastically improve loop performance and lower memory pressure.
