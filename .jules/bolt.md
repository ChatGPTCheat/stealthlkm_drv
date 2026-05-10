## 2024-05-18 - [Capstone Initialization Bottleneck]
**Learning:** Recreating the Capstone disassembly engine per invocation (e.g., inside `disassemble_arm64`) is computationally expensive and causes severe UI stuttering/stalls because the initialization blocks the main thread in `eframe`/`egui`.
**Action:** Initialize Capstone once and cache it in the main application state (`StealthApp`). Even though `capstone::Capstone` does not implement the `Send` trait, it is safe to store it in `StealthApp` since `eframe::App` instances run exclusively on the main UI thread.
