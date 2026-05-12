## 2026-05-12 - Capstone Initialization Bottleneck
**Learning:** Initializing the Capstone disassembly engine per-request in the UI event loop is extremely slow and unnecessary. It doesn't implement Send, so it must be owned by the main UI thread (eframe::App).
**Action:** Cache the Capstone instance within the eframe::App state (StealthApp) to eliminate instantiation overhead during dynamic memory read operations.
