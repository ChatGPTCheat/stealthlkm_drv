## 2024-04-10 - Expensive Capstone Initialization in UI event loop
**Learning:** Initializing the Capstone disassembly engine per memory read request blocks the main UI thread since the event processing loop runs synchronously. Capstone initialization is computationally expensive.
**Action:** Cache the Capstone instance in the `StealthApp` application state during startup and reuse it for all disassembly requests. Since `eframe::App` runs on the main thread, storing `!Send` types like `Capstone` in the app state is perfectly safe and highly performant.
