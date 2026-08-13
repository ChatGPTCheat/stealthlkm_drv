## 2024-08-13 - [Optimize String Allocations in UI Rendering]
**Learning:** In eframe/egui immediate-mode UI rendering loops that execute every frame, using `format!` within loops incurs massive overhead due to continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and the `write!` macro directly on pre-allocated strings (`String::with_capacity`) instead, using import aliasing `use std::fmt::Write as FmtWrite;` to prevent trait collisions.

## 2024-08-13 - [Cache Expensive Objects in UI Thread]
**Learning:** Expensive objects like Capstone disassembly engine can be safely cached in the application state even if they don't implement `Send`, because `eframe::App` instances run on the main UI thread.
**Action:** Move expensive instantiations out of render loops and cache them as options in application state.
