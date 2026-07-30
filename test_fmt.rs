use std::fmt::Write as FmtWrite;
use std::io::{Read, Write as IoWrite};

fn main() {
    let mut out = String::with_capacity(100);
    let _ = writeln!(&mut out, "0x{:X}: {} {}", 0x100, "mov", "x0, x1");
    let _ = write!(&mut out, "0x{:X} ", 0x104);
    println!("{}", out);
}
