1. **Optimize UI rendering loop:** Edit `gui/src/main.rs` to replace `format!` calls inside the per-frame immediate mode loop with `std::fmt::Write` macros (`write!`). This will reduce heap allocations significantly and prevent UI stuttering, especially for the memory hex viewer.
2. **Verify changes:** Run `cd gui && cargo fmt && cargo check && cargo test` to verify there are no compilation errors or logical flaws.
3. **Complete pre-commit steps:** Complete pre-commit steps to ensure proper testing, verification, review, and reflection are done.
4. **Submit PR:** Submit the change with "⚡ Bolt: [performance improvement]" title and required sections.
