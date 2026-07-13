## 2024-07-13 - Avoid format! in Render Loops
**Learning:** Using `format!` inside hot loops (like `egui::Grid` rendering and hex views) incurs significant per-iteration string allocation overhead.
**Action:** Use `std::fmt::Write` with `write!` or `writeln!` on pre-allocated strings to eliminate unnecessary allocations and improve UI frame latency.
