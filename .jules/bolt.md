## 2026-11-09 - eGUI Render Loop String Allocations

**Learning:** This application uses an immediate-mode GUI (eframe/egui) that continuously re-renders (polling at 50ms). Using `format!` inside hot-path rendering loops (like building hex dumps) causes massive, continuous heap allocations and CPU overhead per frame, whereas cold paths (like button clicks) should not be prematurely optimized. Additionally, when using `std::fmt::Write` in Rust files that already import `std::io::Write`, trait naming collisions occur.

**Action:** Always optimize string building in continuous UI render loops by replacing `format!` with `write!` on pre-allocated strings (`String::with_capacity`). Ignore cold paths like button clicks to preserve readability. Use import aliasing (`use std::fmt::Write as _;`) to avoid compiler errors.
