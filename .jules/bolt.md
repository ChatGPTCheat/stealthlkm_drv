## 2024-05-21 - Cached Capstone engine to fix performance issue
**Learning:** `Capstone` engine instantiation inside the event or rendering loop (like `disassemble_arm64`) causes massive performance drops because it triggers expensive native engine re-initialization.
**Action:** Always verify if expensive native context initializations can be hoisted out of loops. Cache `Capstone` inside application state (`StealthApp`) since it doesn't need to implement `Send` across threads as long as it stays on the UI thread.
