## 2026-06-13 - Expensive Capstone Re-initialization
**Learning:** Re-initializing the Capstone disassembly engine on every UI memory read in the `gui` component is a massive performance bottleneck, triggering expensive FFI calls and allocation during the hot `update` loop.
**Action:** Always cache expensive third-party library initializations (especially FFI ones like Capstone) in the persistent application state (`StealthApp`) rather than recreating them on-demand inside event handlers.
