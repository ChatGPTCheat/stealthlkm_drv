
## 2024-07-31 - [Optimize String formatting in egui memory grids]
**Learning:** Using `format!` within egui rendering loops (like printing memory grids byte-by-byte or disassembling instruction streams) causes a massive overhead due to heavy String allocations on every UI frame.
**Action:** When repeatedly appending strings in hot loops (like UI rendering), always use `std::fmt::Write::write!` directly on pre-allocated strings (e.g., `String::with_capacity()`) to eliminate intermediate allocations and boost frame rates.
