## 2024-05-18 - Replacing `format!` within loop in `gui/src/main.rs`
**Learning:** In Rust GUI `main.rs`, inside `disassemble_arm64`, `format!` is used within a loop to build the string: `out.push_str(&format!("0x{:X}: {} {}\n", ...));`. This causes a memory allocation for the intermediate string on every iteration.
**Action:** Use `std::fmt::Write` and `write!(out, "0x{:X}: {} {}\n", ...)` directly on the `String` buffer. Or, `use std::fmt::Write as FmtWrite` as per the memory directive if `std::io::Write` is imported.
