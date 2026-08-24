## 2023-10-25 - Avoid format! in Rust rendering loops
**Learning:** Using `format!` inside frequently executed loops (especially in immediate-mode GUIs like egui) incurs heavy overhead due to continuous string heap allocations.
**Action:** Pre-allocate strings using `String::with_capacity` and format directly into them using `write!` and `std::fmt::Write`.
