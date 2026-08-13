1. **Initialize Capstone in the App State**:
   - Update `StealthApp` struct in `gui/src/main.rs` to cache a `Capstone` instance by adding `cs: Option<Capstone>`.
   - In `StealthApp::new`, initialize the `Capstone` instance with `.arm64()` and `.mode(arch::arm64::ArchMode::Arm)`, and assign it to `cs` wrapped in `Some`.

2. **Optimize String Allocations in Disassembly Rendering Loop**:
   - Modify the signature of `disassemble_arm64` to accept the `&Capstone` instance directly, eliminating the need to re-initialize it on every call.
   - Inside `disassemble_arm64`, replace the `.push_str(&format!(...))` calls within the instruction loop with `std::fmt::Write::write_fmt` (using `write!` macro) to write directly into `out`. Ensure `std::fmt::Write as FmtWrite` is imported.

3. **Optimize String Allocations in Hex Grid Loop**:
   - Refactor the hex viewer code inside the memory tab (`self.memory_tab == 0`).
   - Replace continuous `format!` calls inside the loop for creating hex strings (`dump.push_str(&format!(...))` and `hex_str.push_str(&format!(...))`) using `write!` macro directly onto pre-allocated strings or by creating more performant formatting.

4. **Complete Pre-Commit Steps**:
   - Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.

5. **Submit Changes**:
   - Run `cargo check`, `cargo fmt`, and `cargo test` in `gui/`.
   - Submit the PR with the required `⚡ Bolt: [performance improvement]` format.
