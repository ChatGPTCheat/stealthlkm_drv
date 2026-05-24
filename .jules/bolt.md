## 2024-05-24 - [Capstone Disassembler Initialization Bottleneck]
**Learning:** Initializing the Capstone disassembler engine is an expensive operation and should not be done repeatedly inside event loops or UI updates (e.g., when handling network memory results).
**Action:** Cache the initialized Capstone engine in the application state (e.g., inside StealthApp as Option<Capstone>) to reuse it across memory reads, significantly reducing CPU overhead and potential lag.
