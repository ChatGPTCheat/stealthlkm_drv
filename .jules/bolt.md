## 2024-05-24 - [String Allocations in Egui Rendering Loops]
**Learning:** Using `format!` combined with `push_str` inside immediate-mode GUI (`egui`) rendering loops incurs massive overhead due to continuous, per-iteration string heap allocations on every frame.
**Action:** Always prefer using `std::fmt::Write` with the `write!` macro directly onto pre-allocated strings (via `String::with_capacity`) inside hot rendering loops to eliminate redundant allocations.
