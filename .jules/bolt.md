## 2024-09-27 - Immediate Mode GUI Continuous Rendering
**Learning:** The application uses eframe/egui, an immediate-mode GUI framework. This means rendering functions execute continuously (up to 20 frames per second). Using `format!` inside inner loops within the render path causes massive per-frame heap allocations and garbage collection overhead.
**Action:** Use `String::with_capacity` combined with `std::fmt::Write` (`write!`) directly on the buffer to avoid creating intermediate strings inside high-frequency UI rendering loops.
