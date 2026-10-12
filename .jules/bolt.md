## 2024-05-24 - Capstone Initialization Overhead
**Learning:** Because Capstone initialization is computationally expensive, avoid re-initializing it per call (e.g., in `disassemble_arm64`) and instead cache the instance in the global application state (like `StealthApp`).
**Action:** When working with Capstone in Rust GUI components, initialize it once and store it in the app state to reuse across disassembly calls. Additionally, use `write!` with pre-allocated strings instead of `format!` in loops.
