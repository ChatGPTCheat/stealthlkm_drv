## 2024-05-18 - Avoid format! in immediate-mode UI loops
**Learning:** Using `format!` combined with `push_str` repeatedly inside immediate-mode UI rendering loops (like egui/eframe) incurs massive overhead due to continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` (imported as `FmtWrite` to avoid conflicts with `std::io::Write`) and the `write!` macro directly on pre-allocated strings (`String::with_capacity`) instead of `format!` for better performance.
