## 2024-08-16 - Prevent Continuous Allocation in egui Render Loops
**Learning:** In immediate-mode GUIs like egui (which render continuously), using `format!` macro inside render loops (such as iterating over a memory dump to display hex bytes) causes massive continuous heap allocations per frame, leading to CPU overhead and potential stuttering.
**Action:** Replace `format!` inside loops with `std::fmt::Write` (imported as `FmtWrite` to avoid conflicts with `std::io::Write`) and use `write!` on pre-allocated strings (`String::with_capacity`).
