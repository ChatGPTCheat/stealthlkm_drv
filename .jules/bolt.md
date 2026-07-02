## 2024-05-18 - Caching un-Sendable expensive operations in eframe
**Learning:** Initializing Capstone inside the `disassemble_arm64` function, which is called frequently on memory reads, creates significant overhead and causes unnecessary UI lag.
**Action:** In `eframe` (egui) applications running on a single UI thread, cache expensive un-Sendable dependencies like `Capstone` inside the application struct (e.g. `StealthApp`) to optimize performance and prevent repeated initialization. Use `Option<Capstone>` and initialize it gracefully.
