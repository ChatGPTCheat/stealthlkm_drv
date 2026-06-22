## 2024-05-30 - Capstone Initialization Bottleneck in eframe
**Learning:** Re-initializing the Capstone engine repeatedly during network memory read callbacks causes a severe bottleneck due to high initialization costs. Capstone does not implement `Send`, but since `eframe::App` runs exclusively on the main UI thread, it is entirely safe and necessary to cache `Capstone` inside the application state (`StealthApp`).
**Action:** Always cache expensive, non-thread-safe components like Capstone in the main UI thread state rather than re-creating them in event handling or polling loops.
