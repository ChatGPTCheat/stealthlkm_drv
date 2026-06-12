## 2024-06-12 - Capstone Engine Initialization Overhead
**Learning:** Re-initializing the Capstone disassembly engine repeatedly on every memory read creates severe UI jank in egui, as it involves heavy underlying C library setup/teardown.
**Action:** Always cache expensive C-FFI wrapper instances like Capstone inside the `eframe::App` state (`Option<Capstone>`) when frequent UI polling/rendering depends on them.
