## 2024-05-19 - Cache Capstone Disassembly Engine
**Learning:** Capstone engine initialization in Rust is highly expensive and should not be invoked on hot paths (like during every memory read). It can safely be cached in the application state thread-local variables. Note that `Capstone` lacks `Send`, but it can be maintained inside `StealthApp` since `eframe` GUI apps maintain their app state on the main UI thread.
**Action:** Always cache complex analysis frameworks like Capstone. Initialize them once on app creation and pass references (`&cs`) when executing tasks like disassembly.
