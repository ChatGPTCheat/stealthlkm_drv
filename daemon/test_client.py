import socket
import struct

# Command Enums
CMD_GET_BASE = 1
CMD_READ_MEM = 2

def get_base(sock, mod_name):
    payload = mod_name.encode('utf-8')
    # NetCommand struct: uint32_t cmd_type, uint64_t address, uint32_t size
    # Packed strictly (<IQI)
    header = struct.pack("<IQI", CMD_GET_BASE, 0, len(payload))
    sock.sendall(header + payload)
    
    res_bytes = sock.recv(16)
    if len(res_bytes) < 16:
        print("[-] Connection dropped early")
        return 0
        
    res_cmd, res_addr, res_size = struct.unpack("<IQI", res_bytes)
    return res_addr

def read_mem(sock, address, size):
    header = struct.pack("<IQI", CMD_READ_MEM, address, size)
    sock.sendall(header)
    
    res_bytes = sock.recv(16)
    if len(res_bytes) < 16:
        return b""
        
    res_cmd, res_addr, res_size = struct.unpack("<IQI", res_bytes)
    if res_size == 0:
        print("[-] Read failed (returned size 0)")
        return b""
        
    # Read the data payload directly
    data = sock.recv(res_size, socket.MSG_WAITALL)
    return data

def main():
    print("[*] Connecting to Stealth Daemon over adb forward 1337...")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect(("127.0.0.1", 1337))
    except Exception as e:
        print(f"[-] Failed to connect: {e}")
        return

    # Try finding an Android standard library (like libc) just to test map parser
    base = get_base(s, "libc.so")
    print(f"[+] Server returned Base of libc.so: {hex(base)}")

    if base > 0:
        # Read the ELF header (first 16 bytes)
        elf_header = read_mem(s, base, 16)
        if elf_header:
            print(f"[+] ELF Header Hex:   {elf_header.hex()}")
            print(f"[+] ELF Header ASCII: {elf_header}")
            
            # Check for \x7F ELF magic
            if elf_header.startswith(b"\x7fELF"):
                print("[***] SUCCESS! Read Memory valid!")
            else:
                print("[-] Read memory, but magic is wrong.")

    s.close()

if __name__ == "__main__":
    main()
