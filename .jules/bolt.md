## 2025-04-24 - Capstone Initialization Performance Issue
**Learning:** Initializing the `Capstone` disassembly engine repeatedly on every memory read (in `disassemble_arm64`) causes significant latency because it is an expensive operation. Caching the instance within the application state (e.g., `StealthApp`) avoids this overhead.
**Action:** Always verify if expensive engine/parser instantiations are placed within tight loops or event handlers, and cache them globally or within the application state if they do not require per-invocation configuration.
