#ifndef PROCESSUTILS_H
#define PROCESSUTILS_H
#pragma once
#include <dirent.h>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <stdint.h>
#include <iostream>
#include <algorithm>
#include <cctype>

pid_t FindProcess(const std::string& exe_name);
uintptr_t FindBaseImage(pid_t pid, const std::string& exe_name);

#endif //PROCESSUTILS_H
