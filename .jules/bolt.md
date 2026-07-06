## 2024-05-24 - Rust String Allocation Overhead in Loops
**Learning:** Found a performance anti-pattern in the Rust GUI codebase where `format!` is used inside loops to build strings (e.g., hex dumps, disassembler output). `format!` allocates a new `String` on every iteration, leading to significant memory allocation overhead.
**Action:** Always use the `std::fmt::Write` trait with the `write!` or `writeln!` macros to append directly to pre-allocated `String` buffers inside loops instead of using `string.push_str(&format!(...))`.
