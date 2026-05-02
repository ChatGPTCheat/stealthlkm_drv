## 2024-05-02 - Capstone Instantiation Overhead
**Learning:** Initializing Capstone (`Capstone::new().arm64()...build()`) on every frame/event in Rust is extremely computationally expensive.
**Action:** Always initialize Capstone once and cache it in the application state. It can be safely stored in the `eframe::App` state because it does not implement `Send` but runs on the main UI thread.
