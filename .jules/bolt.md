## 2024-05-08 - Cache Capstone Disassembly Engine
**Learning:** Capstone initialization is computationally expensive and was being instantiated on every single memory read in the GUI. Because `Capstone` doesn't implement the `Send` trait, it cannot be passed between threads, but it can safely be cached in the main eframe `StealthApp` state since it runs on the main UI thread.
**Action:** Always cache expensive parser/disassembler instances in the application state rather than recreating them per-request, keeping thread-safety requirements (`Send`/`Sync`) in mind.
