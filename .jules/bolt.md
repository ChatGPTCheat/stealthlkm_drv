## 2024-05-24 - [Cache Capstone Disassembler Instance]
**Learning:** Capstone engine initialization is computationally expensive. It does not implement `Send`, so it cannot be shared across threads, but it can be safely stored in the main UI thread application state (`eframe::App`).
**Action:** Store `Option<Capstone>` in the `StealthApp` struct instead of re-instantiating it in `disassemble_arm64` on every memory read.
