## 2024-05-24 - Capstone Initialization Bottleneck
**Learning:** In the Rust GUI, initializing the `Capstone` disassembly engine per-request is computationally expensive and causes UI lag during frequent memory updates.
**Action:** `Capstone` does not implement `Send`, but it can be safely cached within `eframe::App` structs (like `StealthApp`) since they are guaranteed to run on the main UI thread. Cache the instance at application startup instead of instantiating it per function call.
