#pragma once
#include <cstdint>

enum CommandType : uint32_t {
    CMD_GET_BASE = 1,
    CMD_READ_MEM = 2,
    CMD_SET_HWBP = 3,
    CMD_CHECK_DEBUG_EVENT = 4,
    CMD_RESUME_HWBP = 5,
    CMD_REMOVE_HWBP = 6,
    CMD_SCAN_AOB = 7,

    // CoreSight SPE Hardware Profiling commands (Zero Detection)
    CMD_SET_SPE_WATCHPOINT = 8,
    CMD_REMOVE_SPE_WATCHPOINT = 9,
    CMD_POLL_SPE_EVENTS = 10
};

#pragma pack(push, 1)
struct NetCommand {
    uint32_t cmd_type;
    uint64_t address;
    uint32_t size;
};
#pragma pack(pop)
