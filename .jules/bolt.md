## 2024-05-18 - [Cache Heavy Initializations outside eframe::update]
**Learning:** `Capstone::new()...build()` is an expensive operation that was previously happening every time memory was read (inside `UiNotify::MemResult`), blocking the UI thread or running redundantly. Egui's immediate mode means state must be carefully managed.
**Action:** Store expensive engines/structs like `Capstone` inside the `eframe::App` state struct (`StealthApp`), initializing them once in the `new` constructor and borrowing them as needed during the application loop.
