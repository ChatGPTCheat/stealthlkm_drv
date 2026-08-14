## 2024-05-18 - Replacing `format!` with `write!` in Rust rendering loops
**Learning:** In eframe/egui, UI loops execute every frame. Using `format!` within loop iterations incurs high overhead due to repeated string allocations on the heap.
**Action:** Replace `format!` with pre-allocated strings (`String::with_capacity()`) and `std::fmt::Write` macro (`write!`) directly on the pre-allocated string buffer for high-frequency operations, minimizing runtime allocations.
