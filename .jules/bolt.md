## 2024-05-24 - Avoid Capstone re-initialization
**Learning:** Initializing the Capstone disassembly engine per call (e.g., in `disassemble_arm64`) is computationally expensive and impacts performance significantly, especially when repeatedly called within the immediate-mode UI or upon every memory read.
**Action:** The Capstone instance should be cached in the global application state (like `StealthApp`) so that initialization overhead is paid only once at startup.

## 2024-05-24 - Avoid format! in eframe/egui UI loops
**Learning:** Using `format!` inside hot loops (like building multi-element strings for hex dumps or disassembler output) in Rust, particularly within `eframe/egui` immediate-mode UI rendering loops, incurs massive overhead from continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and `write!` macro on pre-allocated strings (`String::with_capacity`) to append formatted data. Handle the returned `Result` with `let _ = write!(...);` to suppress warnings. Import `std::fmt::Write as _` to avoid collisions.
