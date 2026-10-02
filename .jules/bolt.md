## 2024-05-24 - eframe/egui format! Performance Bottleneck
**Learning:** Using `format!` inside continuous UI rendering loops (like egui's update) causes severe performance degradation due to thousands of per-frame heap allocations (e.g., in inner loops over memory chunks).
**Action:** Always use `std::fmt::Write` (`write!`) directly on pre-allocated strings (`String::with_capacity`) in immediate-mode GUI hot paths.
