## 2024-10-05 - Avoid Capstone recreation overhead
**Learning:** The `gui` component recreates the `Capstone` disassembly engine (`Capstone::new().arm64()...`) on every memory read (`disassemble_arm64`), which is extremely slow and causes UI thread blocking, defeating eframe's immediate mode responsiveness.
**Action:** The Capstone engine should be initialized once, likely stored inside the `StealthApp` state or lazily initialized, to optimize repeated disassembly operations.

## 2024-10-05 - Optimize egui loop format! allocations
**Learning:** The immediate-mode UI rendering loops use `format!` extensively inside loop bodies (e.g., memory Hex & Pointers Data panel). `format!` allocates a new `String` on the heap for every iteration, creating huge GC overhead for the UI thread, particularly when parsing 1024-byte hex dumps.
**Action:** Replace `format!` in hot rendering loops with `use std::fmt::Write as _` (or similar alias to avoid `io::Write` conflicts) and use the `write!` macro onto a pre-allocated `String::with_capacity` buffer to significantly boost GUI frames per second.
