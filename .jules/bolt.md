## 2023-10-27 - [Network Worker CPU Spin]
**Learning:** In the GUI network worker, standard `try_recv` + `std::thread::sleep` pattern leads to CPU spinning, especially problematic on the main background thread handling both polling events and active UI requests. Egui applications frequently redraw and sleep durations impact overall application latency.
**Action:** Replace `try_recv` + `sleep` with `recv_timeout` computing exact sleep duration down to the next polling event or block indefinitely.
