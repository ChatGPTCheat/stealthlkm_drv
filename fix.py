import re

def fix_file(filepath):
    with open(filepath, 'r') as f:
        content = f.read()

    # We need to optimize `format!` usages inside rendering loops in gui/src/main.rs.
    # Looking at main.rs, there are a few places where format! is used heavily in loops.
    # We can replace them with `use std::fmt::Write;` and `write!` or just use string push if it's simpler,
    # but the prompt specifically says "Use std::fmt::Write and the write! macro directly on pre-allocated strings (String::with_capacity) instead."
    # Wait, the prompt says:
    # "In Rust components, especially within eframe/egui immediate-mode UI rendering loops that execute every frame, avoid using format! within loops. It incurs massive overhead due to continuous per-iteration string heap allocations. Use std::fmt::Write and the write! macro directly on pre-allocated strings (String::with_capacity) instead."
    pass
