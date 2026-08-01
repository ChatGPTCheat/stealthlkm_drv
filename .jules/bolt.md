## 2024-05-30 - Immediate-Mode Rendering Bottlenecks
**Learning:** In eframe/egui immediate-mode UI loops that execute every frame, using `format!` inside iterative rendering loops (like hex dump viewers) incurs massive overhead due to continuous per-iteration string heap allocations.
**Action:** Always pre-allocate with `String::with_capacity()` and use `std::fmt::Write` (`write!`/`writeln!`) macros directly into the buffer instead of `format!` for iterative rendering logic.
