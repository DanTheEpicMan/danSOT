#pragma once

#ifndef MEMORY_H
#define MEMORY_H

#include <string>
#include <sys/types.h>
#include <sys/uio.h>
#include <stdint.h>

// Initialize memory access to the target process
bool InitializeMemoryAccess();

// Memory read/write function templates
template<typename T>
T ReadMemory(pid_t pid, long address);

template<typename T>
T ReadMemory(uintptr_t address);

template<typename T>
bool WriteMemory(pid_t pid, long address, const T& value);

// Read a string from memory with maximum length
std::string ReadString(pid_t pid, long address, size_t maxLength = 128);

// Global variables
extern pid_t ProcessId;
extern long BaseAddress;


#endif //MEMORY_H
