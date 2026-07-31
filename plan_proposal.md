1. **Optimize string formatting in loops in `gui/src/main.rs`**
   - The memory view in `gui/src/main.rs` formats string in loops:
     - In the `📋 Copy All (Hex)` button logic, it uses `format!` to append string for each byte in the raw memory.
     - In the memory grid rendering loop (`while i < self.raw_memory.len()`), it uses `format!` inside `for b in chunk { hex_str.push_str(&format!("{:02X} ", b)); }`.
     - In `disassemble_arm64`, it loops through instructions and uses `format!` to format strings before appending.
   - Using `format!` inside loops allocates a new `String` for every call, which creates a huge memory and CPU overhead, especially since the memory grid renders every UI frame (`update` function of eframe).
   - I will replace these `format!` calls inside loops with `std::fmt::Write` trait's `write!` macro on the target string, which is much more efficient because it doesn't allocate an intermediate string.
   - Add `use std::fmt::Write as FmtWrite;` to the top of `gui/src/main.rs` to avoid conflict with `std::io::Write`.

2. **Verify changes**
   - Read the changes in `gui/src/main.rs`.
   - Run `cd gui && cargo fmt && cargo check && cargo test` to make sure nothing is broken.

3. **Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.**
   - Call `pre_commit_instructions` and follow its instructions to do tests, verifications, reviews and reflections.

4. **Submit the PR**
   - Create a PR titled `⚡ Bolt: [Optimize string formatting in UI loops]` with the appropriate description format as requested by Bolt.
