## 2024-03-24 - [Avoid `format!` in `eframe::egui` render loops]
**Learning:** `format!` inside `egui` render loops causes massive overhead due to continuous heap allocation per-frame.
**Action:** Use `std::fmt::Write` and `write!` on pre-allocated `String::with_capacity()` instead.
