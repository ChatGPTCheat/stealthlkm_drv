#include <iostream>
#include <vector>
#include <string>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#include "memory_engine.h"
#include "net_protocol.h"
#include "map_parser.h"
#include "pid_utils.h"

void HandleClient(int client_fd, MemoryEngine& engine, int target_pid) {
    std::cout << "[+] Client connected. Waiting for commands...\n";
    
    while (true) {
        NetCommand req;
        int bytes = recv(client_fd, &req, sizeof(NetCommand), 0);
        if (bytes <= 0) {
            std::cout << "[-] Client disconnected.\n";
            return;
        }

        NetCommand res = {0};
        res.cmd_type = req.cmd_type;

        switch (req.cmd_type) {
            case CMD_GET_BASE: {
                // Client wants to know the base address of a module
                char mod_name[256] = {0};
                if (req.size > 0 && req.size < sizeof(mod_name)) {
                    recv(client_fd, mod_name, req.size, 0);
                }
                
                uint64_t base = MemoryMapParser::GetModuleBase(target_pid, std::string(mod_name));
                res.address = base;
                res.size = 0;
                send(client_fd, &res, sizeof(NetCommand), 0);
                std::cout << "[*] CMD_GET_BASE: " << mod_name << " -> 0x" << std::hex << base << std::dec << "\n";
                break;
            }
            case CMD_READ_MEM: {
                std::cout << "[*] CMD_READ_MEM Start: 0x" << std::hex << req.address << std::dec << " Size: " << req.size << " bytes\n";
                // Client wants to read memory blocks
                uint32_t needed_chunks = (req.size + 7) / 8; // Calculate how many 8-byte chunks are needed
                std::vector<uint64_t> addrs(needed_chunks);
                for (uint32_t i = 0; i < needed_chunks; ++i) {
                    addrs[i] = req.address + i * 8;
                }
                
                std::vector<uint64_t> chunk_results;
                bool success = engine.ReadBatch(addrs, chunk_results, 8);
                
                if (success && chunk_results.size() == needed_chunks) {
                    res.size = req.size;
                    send(client_fd, &res, sizeof(NetCommand), 0);
                    // Send pure binary payload
                    send(client_fd, chunk_results.data(), req.size, 0); 
                    std::cout << "[+] CMD_READ_MEM Success.\n";
                } else {
                    res.size = 0; // Failure
                    send(client_fd, &res, sizeof(NetCommand), 0);
                    std::cout << "[-] CMD_READ_MEM Failed or partial read.\n";
                }
                break;
            }
            case CMD_SET_HWBP: {
                bool hit = engine.SetHWBP(req.address);
                res.address = hit ? 1 : 0;
                res.size = 0;
                send(client_fd, &res, sizeof(NetCommand), 0);
                std::cout << "[*] CMD_SET_HWBP at 0x" << std::hex << req.address << std::dec << " -> " << hit << "\n";
                break;
            }
            case CMD_CHECK_DEBUG_EVENT: {
                struct StealthPtRegs regs;
                if (engine.CheckDebugEvent(&regs)) {
                    res.size = sizeof(StealthPtRegs);
                    send(client_fd, &res, sizeof(NetCommand), 0);
                    send(client_fd, &regs, sizeof(StealthPtRegs), 0);
                } else {
                    res.size = 0;
                    send(client_fd, &res, sizeof(NetCommand), 0);
                }
                break;
            }
            case CMD_REMOVE_HWBP: {
                bool removed = engine.RemoveHWBP();
                res.address = removed ? 1 : 0;
                res.size = 0;
                send(client_fd, &res, sizeof(NetCommand), 0);
                std::cout << "[*] CMD_REMOVE_HWBP -> " << removed << "\n";
                break;
            }
            case CMD_RESUME_HWBP: {
                struct StealthPtRegs regs;
                if (req.size == sizeof(StealthPtRegs)) {
                    recv(client_fd, &regs, sizeof(StealthPtRegs), MSG_WAITALL);
                    engine.ResumeHWBP(regs);
                    engine.RemoveHWBP(); // Depending on continuous flow, removing HWBP might be needed per hit.
                    res.size = 1; // success
                } else {
                    res.size = 0; // fail
                }
                send(client_fd, &res, sizeof(NetCommand), 0);
                break;
            }
            default:
                std::cout << "[-] Unknown command type: " << req.cmd_type << "\n";
                break;
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <target_executable_name_or_pid|self>\n";
        return 1;
    }

    int target_pid = -1;
    std::string arg1 = argv[1];

    if (arg1 == "self") {
        target_pid = getpid();
    } else {
        // Assume integer PID first
        try {
            target_pid = std::stoi(arg1);
        } catch (...) {
            // Fallback to name resolution
            target_pid = GetPidByName(arg1);
        }
    }

    if (target_pid <= 0) {
        std::cerr << "[-] Failed to resolve target PID for " << arg1 << "\n";
        return 1;
    }

    std::cout << "[*] Routing daemon attached to PID: " << target_pid << "\n";

    MemoryEngine engine(target_pid);
    if (!engine.IsValid()) {
        std::cerr << "[-] MemoryEngine failed to initialize. Root required?\n";
        return 1;
    }
    
    std::cout << "[+] MemoryEngine Zero-Copy layer established!\n";

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0) {
        std::cerr << "[-] Socket creation failed\n";
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(1337);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        std::cerr << "[-] Bind failed on port 1337\n";
        return 1;
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "[-] Listen failed\n";
        return 1;
    }

    std::cout << "[*] Stealth Daemon listening on 0.0.0.0:1337\n";

    while (true) {
        int addrlen = sizeof(address);
        int client_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (client_fd < 0) {
            std::cerr << "[-] Server accept failed\n";
            continue;
        }

        HandleClient(client_fd, engine, target_pid);
        close(client_fd);
    }

    return 0;
}
