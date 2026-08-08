
## 2024-08-08 - egui Immediate Mode String Allocations
**Learning:** In the Rust GUI application using `eframe::egui`, allocating strings via `format!` within the main UI rendering loop incurs massive performance overhead because `egui` executes this loop every single frame. This leads to continuous and redundant string heap allocations, crippling performance, especially when dealing with large buffers like memory views or disassembly text.
**Action:** Always refactor string building inside `egui` rendering loops to use pre-allocated buffers (`String::with_capacity`) combined with `std::fmt::Write` (the `write!` macro). Alias imports to prevent collisions (e.g. `use std::io::{Read, Write as IoWrite}` and `use std::fmt::Write as FmtWrite`).
