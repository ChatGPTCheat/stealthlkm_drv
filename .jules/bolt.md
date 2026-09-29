## 2024-05-19 - GUI Hex Memory Formatting Loop Optimization
**Learning:** Found significant string memory allocations per-frame inside eframe/egui GUI rendering path. Formatting hex strings using `push_str(&format!(...))` in hot loops causes high GC pressure and slow rendering.
**Action:** Replaced `format!` with direct `std::fmt::Write::write!` macro into a pre-allocated `String::with_capacity()` inside hot memory inspection loops to drastically improve performance.
