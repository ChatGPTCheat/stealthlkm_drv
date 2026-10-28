## 2023-10-28 - Egui Render Loop String Formatting
**Learning:** In eframe/egui, the immediate-mode UI rendering loop executes continuously (e.g., 60 times a second). Building multi-element strings like hex dumps using `format!` inside these loops incurs massive overhead from continuous per-iteration string heap allocations.
**Action:** Always use `std::fmt::Write` and `write!` on pre-allocated strings (e.g., `String::with_capacity`) for dynamic string construction within egui hot paths (like `draw_memory_panel`), avoiding `format!`.

## 2023-10-28 - Background Worker Sleep Loop
**Learning:** Using `try_recv` with `std::thread::sleep` for background workers leads to unnecessary CPU wakeups and higher latency.
**Action:** Use `mpsc::Receiver::recv_timeout` instead of `try_recv` combined with `std::thread::sleep` for background workers to minimize CPU usage and improve latency.
