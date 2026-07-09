## 2024-05-24 - Avoid format! inside immediate-mode UI loops
**Learning:** Using `format!` combined with `push_str` inside an egui render loop causes severe performance degradation due to thousands of per-frame heap allocations (e.g., formatting 128 bytes of memory generated 2560 allocations/sec).
**Action:** Replace `format!` in loops with `std::fmt::Write` and `write!` macro on pre-allocated Strings to eliminate per-iteration heap allocations.
