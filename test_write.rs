use std::io::{Read, Write};
use std::fmt::Write as FmtWrite;

fn main() {
    let mut s = String::new();
    let _ = write!(s, "hello {}", "world");
    println!("{}", s);
}
