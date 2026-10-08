## 2023-10-24 - Expensive Initialization in Render/Event Loops
**Learning:** Initializing heavy objects like `Capstone` inside a stateless or frequently called function (like `disassemble_arm64` which is called upon reading memory) incurs unnecessary overhead. Although `Capstone` is not `Send`, it can safely be stored in `eframe::App` state since instances run on the main UI thread.
**Action:** Move expensive initializations out of frequently called functions. Cache them in the application state where possible, and reuse the instance.

## 2023-10-24 - Inefficient String Formatting in Loops
**Learning:** Using `format!` inside a loop to construct a large string or perform hex dumps causes continuous string allocations, heavily impacting performance, especially in hot paths like memory viewing or disassembly formatting.
**Action:** Replace `format!` in loops with `std::fmt::Write` macros (like `write!`) targeting a pre-allocated `String` (`String::with_capacity`).
