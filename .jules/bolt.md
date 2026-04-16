## 2024-04-16 - Cache Capstone Disassembler Initialization
**Learning:** Capstone initialization is computationally expensive and causes performance bottlenecks if instantiated repeatedly on UI events or data updates. Although `Capstone` is `!Send`, it can safely be cached in eframe's `App` struct because `eframe::App` instances run on the main UI thread.
**Action:** Always cache expensive !Send structures like Capstone in the main application state rather than re-initializing them per-function call, especially in hot paths like memory viewing or UI updates.
