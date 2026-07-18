## 2024-05-18 - String formatting in immediate mode UI
**Learning:** In the Rust GUI application (`gui/src/main.rs`) using eframe/egui, `format!` was being used in UI loops to generate strings for displaying hex bytes and addresses. This caused unnecessary per-iteration heap allocations for strings within every frame rendering.
**Action:** Replaced `format!` in hot paths with `std::fmt::Write` on pre-allocated strings (`String::with_capacity`) using the `write!` macro to minimize allocations and improve performance, as required by performance guidelines.
