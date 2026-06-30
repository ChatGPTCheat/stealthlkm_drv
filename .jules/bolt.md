## 2024-06-30 - Caching Expensive Objects in eframe Event Loops
**Learning:** Initializing engines like `Capstone` inside a UI event loop or polling method in an `eframe::egui` application is a significant performance bottleneck due to the high setup cost of the engine.
**Action:** Always cache expensive, non-`Send` objects in the main application state (`StealthApp`), as `eframe::App` runs on the main UI thread, making it safe to store non-`Send` types and reuse them across frames/events.
