use std::io::{Read, Write as IoWrite};

fn main() {
    let mut v = Vec::new();
    v.write_all(b"hello").unwrap();
    println!("{:?}", v);
}
