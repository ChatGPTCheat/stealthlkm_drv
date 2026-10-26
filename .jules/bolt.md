## 2024-10-26 - [eframe immediate-mode UI rendering loops heap allocations]
**Learning:** Avoid using `format!` inside loops to build multi-element strings (e.g., hex dumps). It incurs massive overhead from continuous per-iteration string heap allocations.
**Action:** Use `std::fmt::Write` and `write!` on pre-allocated strings (`String::with_capacity`) instead. However, for single-line UI labels, `format!` is acceptable because passing a reference from a reused buffer forces `egui` to clone the string anyway.

## 2024-10-26 - [Capstone Engine Initialization Overhead]
**Learning:** Capstone initialization is computationally expensive. Re-initializing it per call (e.g., in `disassemble_arm64`) causes massive overhead, especially in rendering loops.
**Action:** Initialize Capstone once and cache the instance (e.g., in the global application state like `StealthApp`).

## 2024-10-26 - [Background Worker Thread Sleep Loop]
**Learning:** Using `rx.try_recv()` combined with `std::thread::sleep` in background worker threads incurs unnecessary CPU usage and higher latency compared to blocking waits when no polling is needed.
**Action:** Use `mpsc::Receiver::recv_timeout` instead of `try_recv` + `sleep` to minimize CPU usage and improve latency.
