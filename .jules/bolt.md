## 2024-04-18 - [Capstone Disassembly Bottleneck]
**Learning:** `Capstone::new()` is computationally expensive to initialize in Rust. Creating a new instance every time memory is read (e.g., in `disassemble_arm64`) creates a severe performance penalty. `Capstone` does not implement the `Send` trait, meaning it cannot be sent between threads.
**Action:** Always pre-initialize `Capstone` once and cache it in a structure that stays on the main thread (like `StealthApp` in `eframe::App`), passing a reference to it whenever disassembly is needed.
