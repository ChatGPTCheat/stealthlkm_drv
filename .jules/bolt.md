## 2025-07-21 - Avoiding Per-Iteration String Allocations in UI Loops
**Learning:** In eframe/egui immediate-mode UI rendering loops, constructing strings per-iteration using `format!` (e.g., inside hex grids or copying memory buffers) incurs massive overhead due to continuous heap allocations on every frame.
**Action:** Pre-allocate strings with `String::with_capacity` based on expected sizes, and use `std::fmt::Write` (e.g., `let _ = write!(&mut string_buffer, ...);`) directly onto the pre-allocated string instead of generating intermediate strings with `format!`.
