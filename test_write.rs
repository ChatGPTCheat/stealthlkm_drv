use std::io::{Read, Write as IoWrite};
use std::fmt::Write as FmtWrite;

fn main() {
    let mut s = String::with_capacity(10);
    let _ = write!(&mut s, "{:02X}", 255);
    println!("{}", s);
}
