## 2024-05-27 - [Format String Overhead in egui Render Loops]
**Learning:** Using `format!` inside `egui` immediate mode render loops (which execute 60 frames per second) triggers excessive continuous heap allocations that significantly degrade rendering performance and garbage collection overhead.
**Action:** Replace `format!` macros with `std::fmt::Write` trait operations (like `write!`) utilizing pre-allocated string buffers (`String::with_capacity`) inside critical render loops to completely eliminate per-frame dynamic allocations.
