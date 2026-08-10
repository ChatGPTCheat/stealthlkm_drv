## 2024-05-24 - eframe/egui per-frame format! anti-pattern
**Learning:** Using `format!` inside tight loops within `eframe::egui` rendering functions (which execute continuously every frame) causes massive continuous heap allocation overhead (e.g., 60,000+ allocations/sec for a simple 1KB hex dump).
**Action:** Always use `std::fmt::Write` with `String::with_capacity` to build strings in egui rendering loops instead of concatenating strings using `format!`.
