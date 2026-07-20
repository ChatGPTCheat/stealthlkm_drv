## 2026-07-20 - [UI Render Loop String Allocations]
**Learning:** In eframe/egui immediate-mode UI rendering loops, using `format!` inside frequent loops (like iterating over memory chunks) causes thousands of unnecessary heap allocations per frame, drastically increasing CPU overhead and frame render times.
**Action:** Always use pre-allocated `String::with_capacity` combined with `write!` (requiring `std::fmt::Write`) to format strings efficiently in render loops, and alias imports (`std::fmt::Write as FmtWrite`) to prevent trait collisions with `std::io::Write`.
