## 2025-02-12 - [Replace format! in rendering loops]
**Learning:** `format!` macros inside tight loops (like egui immediate mode rendering loops which run every frame, or when assembling large strings) cause significant performance degradation due to continuous heap allocations per iteration.
**Action:** Replace `format!` macros inside loops with `std::fmt::Write` macro (`write!`) directly on pre-allocated `String`s (`String::with_capacity`). Use import aliasing `use std::fmt::Write as FmtWrite;` if `std::io::Write` is already imported to avoid naming collisions.
