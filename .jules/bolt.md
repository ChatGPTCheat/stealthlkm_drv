## 2024-04-27 - Capstone Initialization Overhead
**Learning:** `Capstone` is computationally expensive to initialize in the Rust GUI (`gui/`) and shouldn't be recreated on every request. It does not implement the `Send` trait, but can be safely stored in the `eframe::App` main UI thread state (`StealthApp`).
**Action:** Always cache expensive parser/disassembler engine instances inside the main UI state struct when possible instead of re-instantiating them inside utility functions.
