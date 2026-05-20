## 2024-05-24 - Capstone Initialization Bottleneck
**Learning:** Initializing the Capstone disassembly engine is an expensive operation that blocks the main thread in `eframe::egui` applications when placed inside UI rendering or event polling loops like `process_net_events`.
**Action:** Cache the Capstone engine instance within the application state (e.g., `StealthApp` as `Option<Capstone>`) since `eframe::App` instances safely run on the main UI thread.
