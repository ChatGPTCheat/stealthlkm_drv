## 2025-02-18 - Optimized String Formatting in Rust GUI loops
**Learning:** Found string formatting overhead (`format!`) inside immediate mode egui loops in `gui/src/main.rs`. String allocation per-iteration/frame causes high GC/allocation overhead in rust.
**Action:** Replaced `format!` usage inside tight loops and rendering components with `write!` to pre-allocated string buffers.
