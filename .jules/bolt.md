## 2024-05-20 - Capstone Initialization Bottleneck
**Learning:** Initializing the Capstone disassembly engine repeatedly is computationally expensive. It does not implement the `Send` trait, but it can safely be stored in `StealthApp` since `eframe::App` instances run on the main UI thread.
**Action:** Cache the `Capstone` instance in the application state instead of creating a new one on every disassembly request.
