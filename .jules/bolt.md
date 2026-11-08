## 2025-02-23 - [Optimize UI Network Thread and Render loops]
**Learning:** The continuous UI rendering loops and background networking threads frequently cause high CPU usage if not properly gated and if string allocation is performed continuously. Using `std::fmt::Write` pre-allocated buffers vastly outperforms `format!` and string pushing. Using `recv_timeout` instead of `try_recv` with `sleep` efficiently reduces background CPU usage.
**Action:** Always prefer `recv_timeout` and pre-allocated strings with `std::fmt::Write` over busy waiting loops and repeated `format!` on string building.
