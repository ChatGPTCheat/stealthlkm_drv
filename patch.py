import re

with open('gui/src/main.rs', 'r') as f:
    content = f.read()

search = """    loop {
        // Actor requests
        while let Ok(msg) = rx.try_recv() {
            match msg {"""

replace = """    loop {
        let timeout = if hwbp_active {
            let elapsed = last_poll.elapsed();
            if elapsed >= Duration::from_millis(500) {
                Duration::from_millis(0)
            } else {
                Duration::from_millis(500) - elapsed
            }
        } else {
            Duration::from_secs(365 * 24 * 3600)
        };

        // Actor requests
        match rx.recv_timeout(timeout) {
            Ok(msg) => match msg {"""

if search in content:
    content = content.replace(search, replace)

    search2 = """                NetRequest::GdbGetRegisters(cb) => {
                    let _ = cb.send(last_frozen_regs);
                }
            }
        }

        // Background HWBP Polling"""

    replace2 = """                NetRequest::GdbGetRegisters(cb) => {
                    let _ = cb.send(last_frozen_regs);
                }
            },
            Err(std::sync::mpsc::RecvTimeoutError::Disconnected) => break,
            Err(std::sync::mpsc::RecvTimeoutError::Timeout) => {}
        }

        // Background HWBP Polling"""

    if search2 in content:
        content = content.replace(search2, replace2)

        search3 = """            last_poll = Instant::now();
        }
        std::thread::sleep(Duration::from_millis(10));
    }
}"""
        replace3 = """            last_poll = Instant::now();
        }
    }
}"""
        if search3 in content:
            content = content.replace(search3, replace3)
            with open('gui/src/main.rs', 'w') as f:
                f.write(content)
            print("Patched successfully")
        else:
            print("Search 3 failed")
    else:
        print("Search 2 failed")
else:
    print("Search 1 failed")
