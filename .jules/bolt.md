## 2024-05-18 - Caching Capstone in Immediate-Mode UI
**Learning:** Initializing the Capstone disassembly engine is a relatively expensive operation. In the Rust `egui` immediate-mode framework, creating a new `Capstone` instance on every `UiNotify::MemResult` network event (which triggers a UI repaint) causes unnecessary CPU overhead.
**Action:** Always cache expensive state objects (like `Capstone`) in the application state struct (e.g., `StealthApp`) instead of recreating them dynamically within render or event loops.
