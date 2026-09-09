use crate::NetRequest;
use std::fmt::Write as FmtWrite;
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::mpsc;

pub fn run_gdb_server(net_tx: mpsc::Sender<NetRequest>) {
    let listener =
        TcpListener::bind("127.0.0.1:23946").expect("GDB Server failed to bind to 23946");
    println!("[GDB] Server listening on 127.0.0.1:23946 for IDA Pro...");

    for stream in listener.incoming() {
        if let Ok(mut stream) = stream {
            println!("[GDB] Client connected!");
            let tx = net_tx.clone();
            handle_client(&mut stream, tx);
            println!("[GDB] Client disconnected.");
        }
    }
}

fn handle_client(stream: &mut TcpStream, net_tx: mpsc::Sender<NetRequest>) {
    let mut buffer = [0u8; 4096];

    // GDB often sends a global + initially
    stream.write_all(b"+").ok();

    loop {
        let n = match stream.read(&mut buffer) {
            Ok(n) if n > 0 => n,
            _ => return, // Disconnect
        };

        if let Some(packet) = parse_gdb_packet(&buffer[0..n]) {
            // ACK
            stream.write_all(b"+").ok();

            let response = process_gdb_command(&packet, &net_tx);
            if !response.is_empty() {
                let formatted = make_gdb_packet(&response);
                stream.write_all(formatted.as_bytes()).ok();
            } else {
                // Empty implies not supported
                stream.write_all(make_gdb_packet("").as_bytes()).ok();
            }
        } else if buffer[0] == b'\x03' {
            // Ctrl+C interrupt from IDA
            stream.write_all(make_gdb_packet("S05").as_bytes()).ok();
        }
    }
}

fn parse_gdb_packet(data: &[u8]) -> Option<String> {
    if let Some((start, _)) = data.iter().enumerate().find(|(_, &b)| b == b'$') {
        if let Some((end, _)) = data[start..].iter().enumerate().find(|(_, &b)| b == b'#') {
            let actual_end = start + end;
            return String::from_utf8(data[start + 1..actual_end].to_vec()).ok();
        }
    }
    None
}

fn make_gdb_packet(data: &str) -> String {
    let mut checksum = 0u8;
    for b in data.bytes() {
        checksum = checksum.wrapping_add(b);
    }
    format!("${}#{:02x}", data, checksum)
}

fn process_gdb_command(cmd: &str, net_tx: &mpsc::Sender<NetRequest>) -> String {
    if cmd.starts_with("qSupported") {
        // Announce memory map support and target features
        return "PacketSize=1000;qXfer:features:read+;qXfer:memory-map:read+".to_string();
    }
    if cmd.starts_with("qXfer:features:read:target.xml:") {
        let mut xml = String::from(
            r#"<?xml version="1.0"?>
<!DOCTYPE target SYSTEM "gdb-target.dtd">
<target>
  <architecture>aarch64</architecture>
  <feature name="org.gnu.gdb.aarch64.core">
"#,
        );
        for i in 0..=30 {
            xml.push_str(&format!(
                r#"    <reg name="x{}" bitsize="64" type="uint64"/>{}"#,
                i, "\n"
            ));
        }
        xml.push_str(
            r#"    <reg name="sp" bitsize="64" type="data_ptr"/>
    <reg name="pc" bitsize="64" type="code_ptr"/>
    <reg name="cpsr" bitsize="64"/>
  </feature>
</target>"#,
        );
        return format!("l{}", xml);
    }
    if cmd.starts_with("qXfer:memory-map:read:") {
        // Provide a dummy XML memory map indicating the entire 64-bit space is valid RAM
        let xml = "<memory-map>\n<memory type=\"ram\" start=\"0\" length=\"ffffffffffffffff\"/>\n</memory-map>";
        return format!("l{}", xml); // 'l' means last chunk
    }
    if cmd == "qfThreadInfo" {
        return "m1".to_string(); // we only have thread 1
    }
    if cmd == "qsThreadInfo" {
        return "l".to_string(); // end of thread list
    }
    if cmd == "qC" {
        return "QC1".to_string(); // current thread is 1
    }
    if cmd.starts_with("qAttached") {
        return "1".to_string(); // Yes, we are already attached
    }
    if cmd.starts_with("H") {
        return "OK".to_string(); // Set thread commands (Hg0, Hc0)
    }
    if cmd == "?" {
        // Reason for stopping (SIGTRAP)
        return "S05".to_string();
    }
    if cmd == "g" {
        // Read general registers (X0-X30, SP, PC, PSTATE)
        let (cb_tx, cb_rx) = mpsc::channel();
        if net_tx.send(NetRequest::GdbGetRegisters(cb_tx)).is_ok() {
            if let Ok(Some(regs)) = cb_rx.recv() {
                let payload = bytemuck::bytes_of(&regs);
                let mut hex_str = String::with_capacity(payload.len() * 2);
                for &b in payload.iter() {
                    let _ = write!(&mut hex_str, "{:02x}", b);
                }
                return hex_str;
            }
        }

        let mut hex_regs = String::with_capacity(544);
        for _ in 0..272 {
            // 34 registers * 8 bytes = 272 bytes
            hex_regs.push_str("00");
        }
        return hex_regs;
    }
    if cmd.starts_with("m") {
        // mADDR,LENGTH
        let parts: Vec<&str> = cmd[1..].split(',').collect();
        if parts.len() == 2 {
            if let (Ok(addr), Ok(len)) = (
                u64::from_str_radix(parts[0], 16),
                u32::from_str_radix(parts[1], 16),
            ) {
                let (cb_tx, cb_rx) = mpsc::channel();
                if net_tx.send(NetRequest::ReadMem(addr, len, cb_tx)).is_ok() {
                    if let Ok(data) = cb_rx.recv() {
                        let mut hex_str = String::with_capacity(data.len() * 2);
                        for b in data {
                            let _ = write!(&mut hex_str, "{:02x}", b);
                        }
                        return hex_str;
                    }
                }
            }
        }
        return "E01".to_string(); // Error reading
    }

    "".to_string() // Not supported command
}
