## 2023-11-04 - Eliminate per-frame heap allocations in egui rendering loop
**Learning:** Building multi-element strings (like Hex dumps) using `format!` inside `egui` immediate-mode rendering loops causes massive per-frame heap allocations.
**Action:** Always use pre-allocated `String::with_capacity` combined with `std::fmt::Write` (`write!`) for continuous string building in hot rendering paths. Handle the returned `Result` with `let _ = write!(...);`.
