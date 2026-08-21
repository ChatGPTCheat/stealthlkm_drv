## 2024-05-20 - [Avoid per-frame string allocations in eframe/egui]
**Learning:** Continuous string allocations using `format!` inside immediate-mode UI loops (like eframe/egui) cause significant performance overhead.
**Action:** Use `std::fmt::Write` and the `write!` macro directly on pre-allocated strings (`String::with_capacity`) instead of `format!`.
