## 2024-04-02 - Expensive Capstone re-initialization
**Learning:** Initializing the `Capstone` disassembly engine is extremely expensive. Re-initializing it on every disassembly call (e.g. within a UI render loop or upon every memory read) blocks the main thread and introduces severe lag.
**Action:** Always instantiate engines like Capstone once during application startup and cache the instance in the app state.
