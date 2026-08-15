## 2024-05-19 - eframe String Allocation Overhead
**Learning:** Using the `format!` macro within an `eframe` render loop causes significant overhead because it creates implicit string allocations on every single UI frame refresh. This is especially problematic in long loops, such as memory grid viewers.
**Action:** Always pre-allocate strings outside or at the start of loops with `String::with_capacity()` and append to them using `std::fmt::Write` and the `write!` macro to avoid redundant heap allocations.
