## 2024-05-15 - Capstone caching optimization
**Learning:** Initializing Capstone inside the `disassemble_arm64` loop caused significant performance overhead in the UI polling. Caching it in the app state resolves this bottleneck.
**Action:** Always cache expensive immediate-mode UI resources in the global application state when working with `egui` or similar frameworks.
