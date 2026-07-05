🧪 [Testing Improvement] Add robust unit tests for GDB packet parsing

🎯 **What:** The `parse_gdb_packet` function in `gui/src/gdb_server.rs`, which is responsible for extracting valid GDB commands from raw byte streams, lacked unit tests.

📊 **Coverage:** A new test module `tests` has been added covering the following scenarios:
- Valid packets with payload data (`$qSupported#12`).
- Valid packets with empty payload data (`$#00`).
- Packets containing garbage data before the `$` indicator.
- Packets containing garbage data after the `#` indicator.
- Incomplete packets missing the trailing `#`.
- Incomplete packets missing the leading `$`.
- Valid packets format but containing non-UTF8 payload data.

✨ **Result:** Enhanced test coverage around the GDB server network boundary, ensuring that unexpected or malformed inputs from GDB clients (such as IDA Pro) do not cause unexpected panics or incorrectly parsed commands. The testing improvement serves as a safety net for future protocol refactoring.
