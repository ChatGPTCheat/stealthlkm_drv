## 2024-05-14 - Capstone Engine Initialization Overhead
**Learning:** Instantiating the `Capstone` disassembly engine (`Capstone::new()`) on every memory read event introduces significant computational overhead and causes UI jank in the main event loop.
**Action:** The `Capstone` engine should be initialized exactly once during application startup and cached within the application state (e.g., `StealthApp`). It can safely be passed by reference to disassembly routines.
