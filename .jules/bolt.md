## 2024-06-07 - Capstone Disassembly Bottleneck
**Learning:** Initializing the Capstone engine via `Capstone::new().build()` is a computationally expensive operation. In immediate-mode GUIs or event polling loops, repeated initialization causes noticeable UI lag and blocking.
**Action:** Cache the Capstone engine within the persistent application state (`StealthApp`) during startup and pass a reference to it whenever disassembly is needed.
