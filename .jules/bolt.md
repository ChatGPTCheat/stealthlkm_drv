## 2024-10-06 - Replacing format! with write! in egui loops
**Learning:** `format!` inside `egui` immediate mode rendering loops causes significant overhead from continuous heap allocations for string creation every frame.
**Action:** Replace `format!` with `std::fmt::Write` directly on pre-allocated strings or avoid them entirely if egui allows direct formatting, especially in hot paths like the memory hex viewer `egui::Grid`.
## 2024-10-06 - Capstone initialization in rendering loop
**Learning:** Initializing the Capstone engine via `Capstone::new().build()` is an expensive operation. Calling this inside hot paths or continuous polling functions (like when refreshing disassembly views or every time we decode memory) causes massive latency spikes because `Capstone` is repeatedly allocating its internal data structures.
**Action:** Move `Capstone` initialization into `StealthApp::default()` and store it as `cs: Option<Capstone>` in the state. Since egui is immediate mode and runs on a single thread, storing it in the app state avoids `Send` trait issues.
