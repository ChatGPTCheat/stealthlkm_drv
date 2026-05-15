## 2024-05-14 - Capstone GUI performance Optimization
**Learning:** `Capstone` initialization is computationally expensive and shouldn't be recreated on every disassembler call.
**Action:** Store the initialised `Capstone` object in the GUI state and reuse it across disassembler calls.
