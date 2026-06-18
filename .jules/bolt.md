## 2024-05-23 - Disassembler Engine Initialization
**Learning:** Re-initializing the disassembly engine on every memory read causes a performance bottleneck.
**Action:** Cache the engine in the main app struct as Option<Capstone>.
