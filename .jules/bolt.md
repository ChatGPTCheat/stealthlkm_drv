## 2024-05-24 - egui Immediate-Mode String Allocation Overhead
**Learning:** In egui's immediate-mode rendering loop, using `format!` inside loops to build multi-element strings (e.g., for hex dumps) incurs massive overhead due to continuous, per-frame string heap allocations. This is a significant bottleneck for application frame rate.
**Action:** Use `std::fmt::Write` via the `write!` macro on pre-allocated strings (`String::with_capacity()`) for dynamic string construction in render loops, taking care to alias the import (`use std::fmt::Write as _;`) if `std::io::Write` is already in scope.

## 2024-05-24 - Capstone Engine Initialization Overhead
**Learning:** Initializing the Capstone disassembly engine is computationally expensive. Doing it inside the `disassemble_arm64` function which is called every time a read happens is highly inefficient. It's better to instantiate it once and cache it in the application state.
**Action:** Re-use the `Capstone` engine instance by caching it inside `StealthApp` or the actor that needs it.
