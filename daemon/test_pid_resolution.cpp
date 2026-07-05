#include <iostream>
#include <string>
#include <cassert>
#include <unistd.h>
#include <fstream>
#include <sys/types.h>
#include <signal.h>
#include <cstdlib>

#include "pid_utils.h"

void test_pid_resolution() {
    // 1. Test "self"
    assert(ResolvePid("self") == getpid());
    std::cout << "[+] test_pid_resolution: 'self' resolution passed." << std::endl;

    // 2. Test integer PID
    std::string my_pid_str = std::to_string(getpid());
    assert(ResolvePid(my_pid_str) == getpid());
    std::cout << "[+] test_pid_resolution: Integer PID resolution passed." << std::endl;

    // 3. Test invalid PID format (that is also not a process name)
    assert(ResolvePid("invalid_format_xyz123_random_process") == -1);
    std::cout << "[+] test_pid_resolution: Invalid format resolution passed." << std::endl;

    // 4. Test fallback to name resolution using a mock process
    std::string mock_process_name = "mock_daemon_test_proc";

    // Create a simple script that just sleeps, to act as our mock process
    std::ofstream out("mock_script.sh");
    out << "#!/bin/bash\n";
    out << "exec -a " << mock_process_name << " sleep 10\n";
    out.close();

    // Check if system call succeeded but cast to void to ignore the return warning for tests
    (void)!system("chmod +x mock_script.sh");

    pid_t child = fork();
    if (child == 0) {
        // Run the script in child process
        execl("./mock_script.sh", "mock_script.sh", (char*)nullptr);
        exit(1);
    }

    // Wait for the process to be up
    sleep(1);

    int resolved = ResolvePid(mock_process_name);

    // Cleanup regardless of assertion
    kill(child, SIGKILL);
    unlink("mock_script.sh");

    assert(resolved > 0);
    std::cout << "[+] test_pid_resolution: Fallback process name resolution passed (resolved pid: " << resolved << ")." << std::endl;
}

int main() {
    std::cout << "[*] Running tests for stealth_daemon PID resolution..." << std::endl;
    test_pid_resolution();
    std::cout << "[+] All PID resolution tests passed successfully!" << std::endl;
    return 0;
}