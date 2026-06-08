## 2024-05-24 - Cached Disassembly Engine
**Learning:** Instantiating the Capstone disassembly engine on every memory read operation is expensive and causes lag. Storing the Capstone instance in the App state ensures we only pay the cost once at startup.
**Action:** Always cache expensive library instances (like disassembly engines) in the App state rather than creating them on the fly during UI events or polling loops.
