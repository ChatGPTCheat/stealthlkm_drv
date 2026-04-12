
## 2025-02-12 - Capstone Instantiation Bottleneck in GUI Rendering Loop
**Learning:** In the Rust `stealth_gui` app, instantiating a new `Capstone` disassembler engine is computationally expensive. Previously, `disassemble_arm64` created a new `Capstone` engine on every call, leading to UI thread blockages and performance degradation during memory inspection updates.
**Action:** Store the initialized `Capstone` instance inside the `StealthApp` state (`cs: Option<capstone::Capstone>`). The Capstone engine does not implement `Send`, but since the app state lives on the main UI thread via `eframe`, this is safe and resolves the bottleneck.
