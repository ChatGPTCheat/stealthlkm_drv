## 2024-05-23 - [egui String Allocations in Hot Paths]
**Learning:** Using `format!` inside `egui` immediate-mode rendering loops to build multi-element strings (like hex dumps) causes massive overhead from continuous per-iteration string heap allocations.
**Action:** Use `String::with_capacity` and `write!` macro (`std::fmt::Write`) to build strings dynamically within hot UI render loops to minimize allocations.
