1. Modify `gui/src/main.rs` to add `cs: Option<capstone::Capstone>` to `StealthApp`.
2. Initialize `cs` in `StealthApp::new` using `.ok()` instead of `.unwrap()` or `.expect()`.
3. Update `process_net_events` to use `self.cs` for disassembly, replacing calls to `disassemble_arm64`. Remove `disassemble_arm64`.
4. Run `cd gui && cargo fmt && cargo check && cargo test` to ensure changes are correct.
5. Document the performance optimization in `.jules/bolt.md`.
6. Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.
7. Submit the PR using `submit`.
