use std::io::{Read, Write as IoWrite};
use std::fmt::Write as FmtWrite;
use std::net::TcpStream;

fn test_fn(stream: &mut TcpStream) {
    let _ = stream.write_all(b"test");
    let mut s = String::new();
    let _ = write!(s, "hello {}", "world");
}
fn main() {}
