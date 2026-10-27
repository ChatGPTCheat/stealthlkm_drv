## 2024-05-18 - format! in tight UI loops
**Learning:** Using `format!` inside hot UI render loops (like rendering memory hex dumps or disassemblies in egui) forces continuous heap allocations per iteration, slowing down immediate-mode rendering substantially.
**Action:** Use `std::fmt::Write` and `write!` on pre-allocated `String` buffers (`String::with_capacity`) to prevent per-element allocations. Remember to use import aliasing (`use std::fmt::Write as _;`) if `std::io::Write` is already in scope to avoid trait collisions.
