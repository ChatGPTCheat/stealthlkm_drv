## 2024-03-24 - Stealth App Capstone Setup
**Learning:** Capstone initialization is computationally expensive. Re-initializing it per call (e.g., in `disassemble_arm64`) causes significant overhead during rapid UI updates in the eframe/egui loop.
**Action:** Cache the Capstone instance in the global application state (`StealthApp`) instead of recreating it on every disassembly request.
