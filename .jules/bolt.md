## 2024-05-24 - Rust String Format Loop Optimization
**Learning:** Using `format!` inside a tight loop and appending to a `String` with `push_str` is a common anti-pattern that leads to excessive hidden allocations.
**Action:** Replace `out.push_str(&format!(...))` loops with `writeln!(&mut out, ...)` after bringing `std::fmt::Write` into scope. Additionally, use `String::with_capacity(...)` when building large string buffers from iterables where the total size can be roughly estimated to avoid amortized reallocation costs.
