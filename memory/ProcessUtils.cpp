#include "ProcessUtils.h"

// Find the PID of the Proton process running the target exe
pid_t FindProcess(const std::string& exe_name) {
    DIR* proc = opendir("/proc");
    if (!proc) return -1;

    struct dirent* entry;
    while ((entry = readdir(proc)) != nullptr) {
        if (!isdigit(entry->d_name[0])) continue;
        std::string pid_str = entry->d_name;
        std::string cmdline_path = "/proc/" + pid_str + "/cmdline";
        std::ifstream cmdline(cmdline_path);
        if (!cmdline) continue;
        std::string line;
        std::getline(cmdline, line, '\0');
        if (line.find(exe_name) != std::string::npos) {
            closedir(proc);
            return std::stoi(pid_str);
        }
    }
    closedir(proc);
    return -1;
}

uintptr_t FindBaseImage(pid_t pid, const std::string& exe_name) {
    std::string maps_path = "/proc/" + std::to_string(pid) + "/maps";
    std::ifstream maps(maps_path);
    std::string line;
    std::string exe_name_lower = exe_name;
    std::transform(exe_name_lower.begin(), exe_name_lower.end(), exe_name_lower.begin(), ::tolower);

    while (std::getline(maps, line)) {
        std::string line_lower = line;
        std::transform(line_lower.begin(), line_lower.end(), line_lower.begin(), ::tolower);

        // Print lines for debugging
        // std::cout << line << std::endl;

        // Look for the exe name in any form
        if (line_lower.find(exe_name_lower) != std::string::npos) {
            std::istringstream iss(line);
            std::string addr;
            std::getline(iss, addr, '-');
            return std::stoull(addr, nullptr, 16);
        }

        // Fallback: first executable mapping (r-xp)
        if (line.find("r-xp") != std::string::npos && line.find(".exe") != std::string::npos) {
            std::istringstream iss(line);
            std::string addr;
            std::getline(iss, addr, '-');
            return std::stoull(addr, nullptr, 16);
        }
    }
    return 0;
}