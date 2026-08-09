## 2024-05-13 - [Format allocations in Immediate Mode UI loops]
**Learning:** In the Rust GUI `gui/`, using `format!` within eframe/egui immediate-mode UI rendering loops incurs massive overhead due to continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and the `write!` macro directly on pre-allocated strings (`String::with_capacity`) instead, or pre-allocate strings.

## 2024-05-13 - [Capstone instantiation overhead]
**Learning:** In the Rust GUI `gui/`, the `Capstone` disassembly engine is extremely expensive to instantiate. Doing so on every memory read or frame is a significant bottleneck.
**Action:** The `Capstone` engine should be initialized once and stored in the application state.
