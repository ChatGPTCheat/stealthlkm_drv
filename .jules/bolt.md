## 2024-05-18 - Immediate-Mode UI String Allocation Overhead
**Learning:** In eframe/egui, building strings incrementally in rendering loops (like hex dumps) using `format!` causes massive per-frame allocation overhead because the UI loop executes continuously. Button click handlers are cold paths, so avoid prematurely optimizing them.
**Action:** Use `String::with_capacity` and the `write!` macro (with `std::fmt::Write`) to construct multi-element strings in hot rendering loops instead of `format!`.
