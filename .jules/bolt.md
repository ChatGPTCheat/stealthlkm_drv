## 2024-10-17 - Egui Render Loop Allocations
**Learning:** In immediate-mode UI loops (egui), using `format!` inside nested loops (like building hex dumps) causes massive per-iteration heap allocation overhead.
**Action:** Use `String::with_capacity()` and `std::fmt::Write` (`write!`) for concatenating strings in hot render paths instead of `format!`.
