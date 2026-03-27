#pragma once
#include <cstdint>

enum CommandType : uint32_t {
    CMD_GET_BASE = 1,
    CMD_READ_MEM = 2,
    CMD_SET_HWBP = 3,
    CMD_CHECK_DEBUG_EVENT = 4,
    CMD_RESUME_HWBP = 5,
    CMD_REMOVE_HWBP = 6
};

#pragma pack(push, 1)
struct NetCommand {
    uint32_t cmd_type;
    uint64_t address;
    uint32_t size;
    // For CMD_READ_MEM, size represents how many bytes to read.
    // For CMD_GET_BASE, standard payload byte array of `size` follows this struct (module name).
    // For CMD_SET_HWBP, address is used.
    // For CMD_CHECK_DEBUG_EVENT, returns StealthPtRegs if size in response is non-zero.
    // For CMD_RESUME_HWBP, StealthPtRegs payload follows this struct.
};
#pragma pack(pop)
