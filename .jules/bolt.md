## 2026-08-29 - [Avoid format! in UI loops]
**Learning:** Using `format!` inside tight loops (like egui immediate-mode rendering) causes severe performance degradation due to massive per-frame string heap allocations.
**Action:** Pre-allocate string capacity and mutate via `std::fmt::Write` and `write!`/`writeln!` macros to directly stream data into the buffer.
