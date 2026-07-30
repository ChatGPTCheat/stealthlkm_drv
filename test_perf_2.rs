use std::time::Instant;
use std::fmt::Write;

const HEX_CHARS: &[u8] = b"0123456789ABCDEF";

fn main() {
    let mut data = vec![0u8; 1024];
    for i in 0..1024 {
        data[i] = (i % 256) as u8;
    }

    let start = Instant::now();
    for _ in 0..1000 {
        for chunk in data.chunks(8) {
            let mut s = String::new();
            for b in chunk {
                s.push_str(&format!("{:02X} ", b));
            }
        }
    }
    println!("format! took: {:?}", start.elapsed());

    let start = Instant::now();
    for _ in 0..1000 {
        for chunk in data.chunks(8) {
            let mut s = String::with_capacity(chunk.len() * 3);
            for &b in chunk {
                s.push(HEX_CHARS[(b >> 4) as usize] as char);
                s.push(HEX_CHARS[(b & 0x0F) as usize] as char);
                s.push(' ');
            }
        }
    }
    println!("manual took: {:?}", start.elapsed());
}
