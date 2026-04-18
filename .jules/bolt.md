## 2024-05-24 - Capstone Initialization Overhead
**Learning:** Capstone initialization in Rust using `capstone-rs` is computationally expensive. Constructing a new `Capstone` engine per memory read causes significant UI stuttering and CPU overhead.
**Action:** Always cache the `Capstone` engine within the main application state. Despite `Capstone` not implementing `Send`, it is safe to hold within `eframe::App` state since it remains on the main UI thread.
