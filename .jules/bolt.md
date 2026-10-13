## 2024-10-24 - Avoid format! in hot render loops
**Learning:** In immediate-mode GUIs (like `egui`), continuous rendering loops can cause massive heap allocation overhead if `format!` is used to iteratively build multi-element strings (like hex dumps) per frame.
**Action:** Use `std::fmt::Write` with `write!` on a pre-allocated `String::with_capacity` buffer for hot-path string building, while alias-importing `use std::fmt::Write as _;` to prevent conflicts with `std::io::Write`.
