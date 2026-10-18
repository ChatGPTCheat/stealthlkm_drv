## 2025-03-02 - [Reduce CPU usage via recv_timeout and avoid re-initializing Capstone]
**Learning:** Found two architectural bottlenecks specific to this app:
1. `mpsc::Receiver::try_recv` with `std::thread::sleep(10ms)` was being used in background loops, consuming unnecessary CPU and adding latency. Using `recv_timeout` minimizes CPU and awakes exactly when needed.
2. Capstone initialization is extremely expensive and was being performed *every time* memory was read (via `disassemble_arm64`). Moving the `Capstone` instance into `StealthApp` struct cache speeds up rendering significantly.
Also in UI loop, `format!` is expensive, avoiding it in hex rendering loop by using `std::fmt::Write`. But those optimizations were previously mentioned in guidelines.
**Action:** Replace `try_recv()` + sleep with `recv_timeout(duration)`. Cache Capstone instances globally or in App struct instead of per-function-call.
