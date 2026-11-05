## 2024-11-05 - Avoid format! in immediate-mode UI render loops
**Learning:** In Rust immediate-mode UI (egui) render loops and hot paths, using `format!` continuously inside loops for multi-element strings (like hex dumps and disassembly) allocates huge amounts of heap space per-iteration. The proper way to concatenate repetitive formatted data is with `String::with_capacity` combined with `std::fmt::Write` trait's `write!` macro, appending to the pre-allocated string.
**Action:** Replace `push_str(&format!(...))` in hot loops with `let _ = write!(buffer, ...);` and import `std::fmt::Write`.
