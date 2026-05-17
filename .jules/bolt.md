## 2026-05-17 - [Capstone Engine Initialization Bottleneck]
**Learning:** [Capstone is computationally expensive to initialize, causing massive lag if initialized on every memory read loop (e.g., in UI loops). It does not implement the `Send` trait but can be safely stored in the `StealthApp` struct as it runs on the main UI thread.]
**Action:** [Cache the `Capstone` instance in the main app state (`Option<Capstone>`) and pass `Option<&Capstone>` by reference to disassembly functions instead of re-creating it.]
