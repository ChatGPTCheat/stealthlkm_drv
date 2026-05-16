## 2024-05-16 - Expensive Disassembly Engine Initialization
**Learning:** The Capstone disassembly engine is computationally expensive to initialize repeatedly for every memory read event, acting as a significant performance bottleneck. While Capstone does not implement the `Send` trait, in `eframe` GUI applications, `App` instances run entirely on the main UI thread.
**Action:** Always cache expensive non-`Send` dependencies like Capstone within the application state (e.g. `StealthApp`) rather than instantiating them on-demand inside event handlers.
