## 2024-09-24 - Avoid capstone re-initialization in loop
**Learning:** Re-initializing capstone per frame causes huge overhead and is very slow for disassembly. The capstone builder takes significant processing time.
**Action:** Store `Option<Capstone>` in `StealthApp` instead and pass it down.
