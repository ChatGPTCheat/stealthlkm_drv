## 2024-05-14 - Rust String formatting in UI Loops
**Learning:** In eframe/egui UI loops running every frame, using `format!` incurs significant overhead due to constant heap allocations. Using `std::fmt::Write` via `write!` macro onto pre-allocated strings (with `String::with_capacity`) is much more efficient.
**Action:** Use `std::fmt::Write` and `write!` instead of `format!` for frequent string construction in Rust, especially in tight loops or rendering loops. Watch out for trait naming collisions with `std::io::Write` and use import aliasing.
