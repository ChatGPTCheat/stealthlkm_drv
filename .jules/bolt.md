## 2024-10-24 - Stealth App Capstone Engine Cache
**Learning:** Initializing the Capstone engine in `disassemble_arm64` every time data is disassembled is extremely expensive and can cause severe latency and stuttering in the UI render loops or hot network callbacks.
**Action:** Always instantiate large/complex objects (like `Capstone`) once within the persistent App state (`StealthApp`) and pass a reference to them during rendering.
