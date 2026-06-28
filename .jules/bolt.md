## 2026-06-28 - Cache Capstone initialization
**Learning:** Initializing the Capstone disassembly engine within the `eframe` GUI render/event loop causes significant lag due to the high cost of creating the engine repeatedly for every disassembly update.
**Action:** Cache the initialized `Capstone` instance in the application state struct (e.g., `StealthApp`) to optimize performance and prevent unnecessary re-initializations during the UI loop.
