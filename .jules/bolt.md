
## 2024-05-24 - [Avoid `format!` in egui render loops]
**Learning:** In Rust GUI components using `eframe::egui`, using `format!` within the immediate-mode UI rendering loops incurs massive overhead due to continuous per-iteration string heap allocations. The string allocations happen every frame (up to 60+ times a second).
**Action:** Use `std::fmt::Write` and the `write!` macro directly on pre-allocated strings (`String::with_capacity`) instead of `format!` for frequent string formatting inside render loops or large collection iterations.
