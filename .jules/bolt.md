## 2025-02-12 - Cached Capstone Initialization
**Learning:** Capstone initialization is computationally expensive. It was previously being initialized on every `disassemble_arm64` call.
**Action:** Store the initialized `Capstone` instance in the `StealthApp` struct to reuse it across disassembly requests, significantly improving performance.
