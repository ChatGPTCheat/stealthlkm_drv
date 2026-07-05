#include "pid_utils.h"
#include <dirent.h>
#include <fstream>
#include <string>
#include <cstdlib>
#include <algorithm>
#include <cctype>

int GetPidByName(const std::string& name) {
    DIR* dir = opendir("/proc");
    if (!dir) return -1;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_DIR) {
            std::string dirName = entry->d_name;
            if (std::all_of(dirName.begin(), dirName.end(), ::isdigit)) {
                int pid = -1;
                try {
                    pid = std::stoi(dirName);
                } catch (...) {
                    continue;
                }

                // Check comm
                std::string commPath = "/proc/" + dirName + "/comm";
                std::ifstream commFile(commPath);
                std::string commName;
                if (commFile && std::getline(commFile, commName)) {
                    if (commName == name) {
                        closedir(dir);
                        return pid;
                    }
                }

                // Fallback to cmdline for 15-char truncation limit of comm files
                std::string cmdlinePath = "/proc/" + dirName + "/cmdline";
                std::ifstream cmdlineFile(cmdlinePath);
                std::string cmdlineName;
                if (cmdlineFile && std::getline(cmdlineFile, cmdlineName, '\0')) {
                    size_t pos = cmdlineName.find_last_of('/');
                    std::string baseName = (pos == std::string::npos) ? cmdlineName : cmdlineName.substr(pos + 1);
                    if (baseName == name || cmdlineName == name) {
                        closedir(dir);
                        return pid;
                    }
                }
            }
        }
    }
    closedir(dir);
    return -1;
}
