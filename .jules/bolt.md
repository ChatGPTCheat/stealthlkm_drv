## 2024-05-24 - Egui Render Loop Allocations
**Learning:** Immediate-mode GUIs like eframe/egui execute rendering functions every frame (e.g., 60 FPS). Using `format!` inside nested loops (like formatting hex bytes in memory views) within these functions causes massive, continuous string heap allocations, placing severe pressure on the memory allocator.
**Action:** Always pre-allocate with `String::with_capacity` and use the `write!` macro (`std::fmt::Write`) to append directly to the buffer in immediate-mode UI rendering loops.
