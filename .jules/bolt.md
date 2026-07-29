## 2024-11-20 - String Formatting in Egui Rendering Loops
**Learning:** In Rust eframe/egui UI components, `format!` allocates a new `String` every iteration of the immediate-mode rendering loop. When processing collections (like dumping memory hex), using `format!` rapidly allocates and drops strings, causing a significant performance bottleneck.
**Action:** Use `String::with_capacity` and `std::fmt::Write::write_fmt` instead of `format!` for assembling strings dynamically inside hot loops. Use import aliasing (`use std::fmt::Write as FmtWrite;`) if `std::io::Write` is already imported.
