## 2024-07-05 - Capstone Initialization Performance Bottleneck
**Learning:** Initializing the Capstone disassembly engine dynamically upon every memory read is extremely slow and blocks the UI thread/event loop, making the application appear unresponsive when continuously reading memory or polling.
**Action:** Cache expensive structs like the Capstone engine within the main application state (`StealthApp` in `egui`) during startup and reuse the instance across updates instead of re-instantiating it.
