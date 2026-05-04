## 2024-05-04 - Capstone Initialization Overhead
**Learning:** The Capstone disassembly engine is computationally expensive to initialize and doing so on every memory read causes a performance bottleneck. Additionally, it does not implement the `Send` trait, which prevents sending it across threads.
**Action:** Cache the `Capstone` instance within the application state (e.g., `StealthApp` as `Option<Capstone>`), as `eframe::App` instances run safely on the main UI thread.
