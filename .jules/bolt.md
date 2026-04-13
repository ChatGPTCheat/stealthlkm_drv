## 2023-10-24 - Capstone Initialization Bottleneck
**Learning:** Initializing the Capstone disassembly engine is computationally expensive. Doing it on every disassembly request causes unnecessary overhead and UI blocking.
**Action:** Cache the `Capstone` instance in the application state. Even though `Capstone` doesn't implement the `Send` trait, it can be safely stored in the `eframe::App` (e.g., `StealthApp`) since instances run entirely on the main UI thread.
