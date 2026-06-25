## 2024-05-24 - [Performance Optimization]
**Learning:** The Rust application uses an immediate mode UI framework. Expensive object instantiations inside render or utility functions cause significant overhead.
**Action:** Initialize expensive objects once and cache them within the application state struct.
