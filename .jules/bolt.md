## 2025-02-28 - Optimized Rust GUI String Formatting inside Immediate-Mode Rendering Loop

**Learning:** In Rust GUI applications using `eframe::egui`, immediate-mode rendering continuously calls the UI update functions (at typically 60 FPS). Using `format!` inside these hot paths (like looping over memory chunks to create hex strings or assembling massive disassembly text) causes severe continuous memory allocation, tanking performance.

**Action:** Replaced `format!` inside loops with pre-allocated strings (`String::with_capacity`) and `std::fmt::Write::write!` to minimize dynamic allocations during the render loop. Also used `writeln!` where appropriate. Documented that `IoWrite` and `FmtWrite` trait collisions are easily resolved by aliasing their imports.
