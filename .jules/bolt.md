## 2024-05-24 - [Avoid format! in UI loops]
**Learning:** In the Rust `eframe::egui` GUI, using `format!` within continuous UI rendering loops or hot paths incurs significant overhead due to string heap allocations.
**Action:** Replace `format!` with `write!` or `writeln!` on pre-allocated strings (`String::with_capacity`) using `std::fmt::Write`.
