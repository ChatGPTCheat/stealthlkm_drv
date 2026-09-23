## 2024-09-23 - Avoid format! in loops
**Learning:** Using `format!` within inner hot loops inside `egui` memory inspector causes excessive allocations on each UI render frame. The same pattern is present in the `gdb_server` handlers.
**Action:** Replace string concatenation using `format!` with `std::fmt::Write` directly on pre-allocated strings (`String::with_capacity`) instead to avoid allocation overhead in continuous polling and UI loops.
