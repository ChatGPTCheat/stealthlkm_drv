## 2026-10-19 - Avoid continuous per-iteration string heap allocations
**Learning:** In Rust components, especially within eframe/egui immediate-mode UI rendering loops, using `format!` inside loops to build multi-element strings (e.g., hex dumps) incurs massive overhead from continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and `write!` on pre-allocated strings (`String::with_capacity`) instead of `format!`. Use import aliasing if trait collisions occur.
