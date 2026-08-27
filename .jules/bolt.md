## 2024-05-24 - eframe/egui render loop allocations
**Learning:** Using `format!` inside `egui` rendering loops causes continuous per-iteration string heap allocations since the loop runs every frame. This leads to massive overhead, especially when iterating over memory chunks where hundreds of `format!` calls occur per frame (e.g., in `draw_memory_panel`).
**Action:** Always pre-allocate strings (`String::with_capacity`) and use `std::fmt::Write` (with `write!`) for string formatting inside UI rendering loops.
