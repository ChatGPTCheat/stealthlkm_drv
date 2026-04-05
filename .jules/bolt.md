## 2024-04-05 - Capstone Disassembler Initialization Overhead
**Learning:** `Capstone::new().build()` is computationally expensive and causes noticeable lag in the GUI if invoked per memory read operation. It must be cached to maintain real-time performance.
**Action:** Cache the `Capstone` instance in the main application state (`StealthApp`) during initialization and pass it by reference to `disassemble_arm64` to prevent continuous reallocation and engine setup overhead.
