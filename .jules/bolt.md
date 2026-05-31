## 2024-05-31 - Cache Capstone instantiation
**Learning:** Instantiating `Capstone::new()` is an expensive operation that can cause lag if done repeatedly.
**Action:** Always cache expensive initialization objects in the long-lived struct (like `StealthApp`) rather than inside functions that are called frequently or during event polling.
