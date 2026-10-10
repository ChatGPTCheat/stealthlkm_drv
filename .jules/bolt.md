## 2024-10-10 - Avoid format! in egui render loops
**Learning:** In eframe/egui immediate-mode UI render loops, using `format!` inside inner loops to build multi-element strings (like hex dumps) causes massive per-frame heap allocations, degrading rendering performance.
**Action:** Use `String::with_capacity` and `write!` macro from `std::fmt::Write` to build strings in hot UI loops efficiently.
