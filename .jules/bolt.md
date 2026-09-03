## 2026-09-03 - Immediate-mode UI String Formatting Overhead
**Learning:** In eframe/egui (immediate-mode UI), formatting strings inside loops (like hex rendering in memory viewers) executes every frame (60+ FPS), causing massive string allocation overhead.
**Action:** Pre-allocate strings using `String::with_capacity` and use `std::fmt::Write` (`write!`) directly to the buffer to avoid per-element temporary allocations and reduce CPU utilization in render loops.
