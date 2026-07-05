#ifndef PID_UTILS_H
#define PID_UTILS_H

#include <string>

int GetPidOfProcess(const std::string& process_name);
int ResolvePid(const std::string& arg1);

#endif // PID_UTILS_H
