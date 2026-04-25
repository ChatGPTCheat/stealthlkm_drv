## 2024-05-24 - Capstone Caching
**Learning:** The Rust GUI (`gui/`) uses the `eframe::egui` framework and the `Capstone` disassembly engine. `Capstone` is computationally expensive to initialize and is currently re-initialized every time memory is disassembled.
**Action:** Cache the `Capstone` instance within the application state (e.g., `StealthApp`). `Capstone` does not implement the `Send` trait, but can be safely stored in `StealthApp` since `eframe::App` instances run on the main UI thread.
