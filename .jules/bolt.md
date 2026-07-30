## 2024-07-30 - [egui Immediate-Mode UI Allocations]
**Learning:** Using `format!` inside `egui` rendering loops causes severe performance degradation due to massive per-frame heap allocations for temporary strings, particularly in high-volume areas like hex viewers or disassembly dumps.
**Action:** When updating `egui` UI loops, replace all `format!` macro calls with pre-allocated `String::with_capacity` buffers and utilize `std::fmt::Write` macros (`write!`, `writeln!`) to append data without reallocating per element.
