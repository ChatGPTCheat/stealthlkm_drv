## 2024-10-21 - Memory allocation loop performance overhead
**Learning:** Found several places where the `format!` macro is used inside tight loops to append hexadecimal representations of byte sequences into a dynamically sized `String`. This is extremely inefficient due to repeated heap allocations and the performance overhead of the formatting macro.
**Action:** Replace `format!` usage inside tight loops appending strings with the `std::fmt::Write` trait (`use std::fmt::Write as _;`) and the `write!` macro operating on pre-allocated buffers.
