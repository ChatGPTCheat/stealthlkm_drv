## 2024-05-30 - [Capstone Instantiation Bottleneck]
**Learning:** Re-instantiating the `Capstone` disassembler engine on every memory read creates significant UI stalling and overhead in the Rust eframe application.
**Action:** Cache the `Capstone` instance directly in the UI application state (`StealthApp`), initializing it once at startup and passing it by reference to the disassembly routines.
