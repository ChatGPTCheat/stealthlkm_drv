use std::io::{Read, Write};
use std::fmt::Write as FmtWrite;

fn main() {
    let mut dump = String::with_capacity(100);
    let _ = write!(&mut dump, "Hello {:02X}", 10);
    println!("{}", dump);
}
