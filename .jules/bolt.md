## 2024-05-07 - Capstone Initialization Performance Issue
**Learning:** Initializing the Capstone disassembly engine repeatedly is computationally expensive and significantly degrades GUI performance when interpreting raw memory streams live, because `disassemble_arm64` creates a new instance every time.
**Action:** Cache the initialized `Capstone` instance in the application state (`StealthApp`), taking into account that it does not implement the `Send` trait, but can safely reside in `StealthApp` since `eframe::App` instances run exclusively on the main UI thread.
