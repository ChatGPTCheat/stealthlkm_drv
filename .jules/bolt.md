## 2025-05-13 - Capstone Disassembly Overhead in GUI
**Learning:** Capstone::new() is called on every memory read inside disassemble_arm64(), which is an expensive operation in the egui framework, especially since it's triggered by GUI interactions/polling.
**Action:** Cache the Capstone instance inside the StealthApp struct to avoid reinitializing the disassembler on every UI update.
