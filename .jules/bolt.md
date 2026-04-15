## 2024-05-18 - Capstone Re-initialization
**Learning:** Re-initializing the Capstone disassembly engine on every UI frame/memory read is extremely expensive and causes lag in the Rust eframe app. Capstone is not `Send` but can be safely stored in the `StealthApp` state since it only runs on the main thread.
**Action:** Always cache Capstone initialization at the app state level instead of inside frequent update/render loops.
