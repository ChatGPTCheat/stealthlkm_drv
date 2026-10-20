## 2024-04-04 - [GUI Performance Optimization]
**Learning:** `Capstone` disassembly engine initialization in `gui/src/main.rs` was happening on every memory read event (`disassemble_arm64` called `Capstone::new()`). This is computationally expensive.
**Action:** Move `Capstone` initialization to `StealthApp::new()` and cache it as an `Option<capstone::Capstone>` field inside `StealthApp`. The `disassemble_arm64` function should take a reference to this cached instance.
