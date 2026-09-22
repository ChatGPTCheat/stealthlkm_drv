## 2024-05-24 - eframe immediate mode string format bottleneck
**Learning:** In eframe/egui, UI loops execute every frame. Using `format!` within iterators in these UI loops incurs massive overhead due to continuous heap allocations for strings, completely blocking the hotpath in larger collections.
**Action:** Always pre-allocate strings outside of iterators with `String::with_capacity` and use `std::fmt::Write` combined with the `write!` macro for rendering data inside eframe hot loops.
