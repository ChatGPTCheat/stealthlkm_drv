## 2026-06-19 - Cache Capstone Disassembly Engine
**Learning:** Re-initializing the Capstone engine on every memory read incurs significant overhead and blocks the UI thread during FFI setup.
**Action:** Initialize Capstone exactly once during application state creation (e.g., in `StealthApp::new`) and cache it as `cs: Option<Capstone>` to avoid repetitive FFI boundary initialization overhead.
