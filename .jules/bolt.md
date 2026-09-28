## 2025-02-28 - Optimized mpsc background worker CPU usage
**Learning:** Found a hot path where `try_recv()` inside a continuous `loop` with a short `std::thread::sleep(Duration::from_millis(10))` was needlessly spinning and consuming CPU. The `recv_timeout` method allows for blocking wait up to a computed timeout (based on the HWBP poll interval), minimizing CPU usage and latency.
**Action:** Always prefer `recv_timeout` or `recv` over `try_recv` with a sleep loop when a thread mainly waits for channel messages but needs occasional time-based polling.
