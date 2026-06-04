## 2024-11-20 - Capstone Engine Initialization Bottleneck
**Learning:** The Rust `egui` immediate mode framework evaluates UI logic rapidly (often 60 FPS). Initializing a complex engine like `Capstone` inside a recurring data update or polling loop causes a massive overhead.
**Action:** Expensive system/engine initializations (like `Capstone`) must be parsed and stored within the application state (e.g., `StealthApp`) exactly once at startup, allowing for repeated borrows instead of expensive recreation.
