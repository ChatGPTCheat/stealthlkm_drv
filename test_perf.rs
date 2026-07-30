use std::time::Instant;
use std::fmt::Write;

fn main() {
    let mut data = vec![0u8; 1024];
    for i in 0..1024 {
        data[i] = (i % 256) as u8;
    }

    let start = Instant::now();
    for _ in 0..1000 {
        let mut hex_str = String::new();
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
        let mut hex_str = String::new();
        for chunk in data.chunks(8) {
            let mut s = String::with_capacity(chunk.len() * 3);
            for b in chunk {
                let _ = write!(&mut s, "{:02X} ", b);
            }
        }
    }
    println!("write! took: {:?}", start.elapsed());
}
