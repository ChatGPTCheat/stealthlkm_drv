## 2025-01-20 - [Performance string formatting in Rust]
**Learning:** [Using `format!` inside immediate-mode GUI loops (eframe/egui) incurs massive overhead due to continuous heap allocations. `std::fmt::Write` should be used to write into a single string to prevent excessive allocations.]
**Action:** [I have replaced multiple `format!` occurrences inside loops with `std::fmt::Write` into strings with pre-allocated capacity `String::with_capacity()`.]
