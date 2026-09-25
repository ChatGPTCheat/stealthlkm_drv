## 2026-09-25 - Prevent String Allocations in Immediate Mode UIs
**Learning:** The eframe/egui GUI renders every frame, meaning 'format!' macros inside rendering loops execute continuously, causing massive overhead due to constant string heap allocations. The application caches Capstone to avoid expensive recreation, but left formatting allocations unchecked in hot paths like the memory hex view.
**Action:** Always replace 'format!' inside immediate mode rendering loops with 'write!' on pre-allocated strings using 'String::with_capacity' to recycle memory efficiently.
