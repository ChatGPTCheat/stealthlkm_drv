
## 2024-05-18 - Immediate-Mode GUI String Allocation Overhead
**Learning:** In eframe/egui immediate-mode UI rendering loops that execute every frame, using `format!` within loops incurs massive overhead due to continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and the `write!` macro directly on pre-allocated strings (`String::with_capacity`) instead of `format!` for constructing dynamic text inside render/processing loops.
