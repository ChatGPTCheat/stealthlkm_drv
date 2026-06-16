## 2024-06-16 - Expensive Capstone Re-initialization
**Learning:** The Rust GUI application rebuilds the Capstone disassembly engine from scratch every single time `disassemble_arm64()` is called. Capstone initialization is relatively expensive and this function is called on every memory read result, causing unnecessary performance overhead.
**Action:** The `Capstone` engine should be initialized once and cached inside the `StealthApp` state (`cs: Option<Capstone>`). This avoids rebuilding the engine repeatedly and improves the responsiveness of the disassembly view.
