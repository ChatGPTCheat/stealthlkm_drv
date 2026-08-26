## 2024-11-20 - [Optimizing String Formatting in Immediate Mode UIs]
**Learning:** Using `format!` inside eframe/egui UI loops or tight inner loops (like disassembling instructions) incurs a high performance cost due to continuous heap allocations.
**Action:** Replace `format!` with `std::fmt::Write` (e.g. `write!(&mut string, ...)`) on pre-allocated strings (`String::with_capacity()`) where possible, especially in high-frequency rendering or processing loops.
