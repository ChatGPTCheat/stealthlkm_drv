## 2024-05-18 - Caching Capstone Initialization
**Learning:** Initializing `Capstone` inside a frequently called function (`disassemble_arm64`) causes significant overhead and potential stuttering in the `eframe::egui` UI loop.
**Action:** Cache the `Capstone` instance in the application state (`StealthApp`) during initialization, handling `Result` safely without `.unwrap()` to prevent startup crashes.
