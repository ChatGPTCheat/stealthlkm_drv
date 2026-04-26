## 2024-05-14 - Capstone Caching Optimization
**Learning:** `Capstone` engine initialization is computationally expensive. Re-creating it dynamically during memory reads causes unnecessary overhead and can lead to UI stutters.
**Action:** Always cache expensive parser/disassembler instances in the application state instead of instantiating them dynamically per-operation. Since `Capstone` isn't `Send`, store it directly in the `eframe::App` state which runs on the main UI thread.
