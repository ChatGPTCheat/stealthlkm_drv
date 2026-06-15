
## 2024-06-15 - Cache expensive Capstone initialization
**Learning:** The Capstone disassembly engine is expensive to initialize. Recreating it on every memory read causes significant UI stuttering and CPU overhead in immediate mode frameworks.
**Action:** Cache the initialized Capstone instance in the application state struct (e.g., `StealthApp`) during startup to reuse it across frames.
