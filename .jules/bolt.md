## 2024-05-24 - Capstone Initialization Overhead
**Learning:** Initializing the Capstone disassembly engine per-request in the immediate mode UI event loop causes significant latency during rapid memory polling.
**Action:** Cache the `Capstone` engine inside the main `StealthApp` state using `Option<Capstone>` and reuse it across disassembly requests. Ensure graceful handling without `.unwrap()` to avoid startup panics.
