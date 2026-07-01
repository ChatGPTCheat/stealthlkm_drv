## 2024-07-01 - Cache Capstone Initialization
**Learning:** Expensive Rust object creation (like Capstone engine) inside hot code paths or frequent network response handlers leads to noticeable performance drops.
**Action:** Instantiate expensive objects once as part of the app state and reuse them across calls.
