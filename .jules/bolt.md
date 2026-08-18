## 2024-05-18 - [Rust Egui String Formatting Overhead]
**Learning:** In Rust eframe/egui UI components, continuously creating strings via `format!` inside hot render loops generates significant heap allocation overhead, dropping framerates or increasing latency unnecessarily.
**Action:** Replace `format!` in loops with `std::fmt::Write` via the `write!` macro onto pre-allocated strings (`String::with_capacity`), ensuring `std::fmt::Write` is appropriately import-aliased if `std::io::Write` is also in scope to prevent trait collisions.
