#include "pid_utils.h"
#include <unistd.h>
#include <cstdio>
#include <stdexcept>

#include <algorithm>
#include <cctype>

// Check if string contains only valid characters for a process name
bool IsValidProcessName(const std::string& name) {
    return std::all_of(name.begin(), name.end(), [](char c) {
        return std::isalnum(c) || c == '_' || c == '-' || c == '.';
    });
}

// For simplicity, we implement a naive pidof using shell
int GetPidOfProcess(const std::string& process_name) {
    if (!IsValidProcessName(process_name)) {
        return -1;
    }
    char buf[512];
    std::string cmd = "pidof -s " + process_name;
    FILE *cmd_pipe = popen(cmd.c_str(), "r");
    if (!cmd_pipe) return -1;
    if (fgets(buf, 512, cmd_pipe) != NULL) {
        pclose(cmd_pipe);
        return std::stoi(buf); // Converts string PID to int
    }
    pclose(cmd_pipe);
    return -1;
}

int ResolvePid(const std::string& arg1) {
    int target_pid = -1;
    if (arg1 == "self") {
        target_pid = getpid();
    } else {
        // Assume integer PID first
        try {
            target_pid = std::stoi(arg1);
        } catch (...) {
            // Fallback to name resolution
            target_pid = GetPidOfProcess(arg1);
        }
    }
    return target_pid;
}
