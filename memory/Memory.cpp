#include "Memory.h"

#include <iostream>
#include <cstring>
#include <signal.h>
#include <vector>

// Global variables
pid_t ProcessId = 0;
long BaseAddress = 0;

// Memory read/write function templates implementation
template<typename T>
T ReadMemory(pid_t pid, long address)
// {
//     T buffer;
//     struct iovec local[1];
//     struct iovec remote[1];
//
//     local[0].iov_base = &buffer;
//     local[0].iov_len = sizeof(T);
//     remote[0].iov_base = (void*)address;
//     remote[0].iov_len = sizeof(T);
//
//     ssize_t nread = process_vm_readv(pid, local, 1, remote, 1, 0);
//     if (nread != sizeof(T))
//     {
//         // Silent failure - don't spam console
//         memset(&buffer, 0, sizeof(T));
//     }
//
//     return buffer;
// }
{
    T buffer;
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = &buffer;
    local[0].iov_len = sizeof(T);
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = sizeof(T);

    ssize_t nread = process_vm_readv(pid, local, 1, remote, 1, 0);
    if (nread != sizeof(T))
    {
        std::cerr << "[ReadMemory] Failed to read " << sizeof(T)
                  << " bytes at address 0x" << std::hex << address
                  << " (pid " << pid << "). nread=" << nread << std::endl;
        perror("[ReadMemory] process_vm_readv");
        memset(&buffer, 0, sizeof(T));
    }

    return buffer;
}

// Implementation for GameData.h template
template<typename T>
T ReadMemory(uintptr_t address)
{
    return ReadMemory<T>(ProcessId, address);
}

template<typename T>
bool WriteMemory(pid_t pid, long address, const T& value)
{
    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = (void*)&value;
    local[0].iov_len = sizeof(T);
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = sizeof(T);

    ssize_t nwrite = process_vm_writev(pid, local, 1, remote, 1, 0);
    return (nwrite == sizeof(T));
}

// Read a string from memory with maximum length
std::string ReadString(pid_t pid, long address, size_t maxLength)
{
    std::vector<char> buffer(maxLength, 0);

    struct iovec local[1];
    struct iovec remote[1];

    local[0].iov_base = buffer.data();
    local[0].iov_len = maxLength;
    remote[0].iov_base = (void*)address;
    remote[0].iov_len = maxLength;

    ssize_t nread = process_vm_readv(pid, local, 1, remote, 1, 0);
    if (nread <= 0)
    {
        return "";
    }

    // Ensure null-termination
    buffer[maxLength - 1] = '\0';
    return std::string(buffer.data());
}
// Initialize memory access to the target process
bool InitializeMemoryAccess()
{

    // Check if we have a valid process ID
    if (ProcessId <= 0)
    {
        std::cerr << "Invalid process ID" << std::endl;
        return false;
    }

    // Test memory access
    std::cout << "Testing memory access at base address..." << std::endl;
    try {
        int testValue = ReadMemory<int>(ProcessId, BaseAddress);
        std::cout << "Memory read successful" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Memory read failed: " << e.what() << std::endl;
        return false;
    }

    // If the process has crashed or exited, this will fail
    if (kill(ProcessId, 0) != 0)
    {
        std::cerr << "Target process is not accessible" << std::endl;
        return false;
    }

    std::cout << "Memory access initialized successfully" << std::endl;
    return true;
}

// Explicit template instantiations for common types
template int ReadMemory<int>(pid_t pid, long address);
template long ReadMemory<long>(pid_t pid, long address);
template float ReadMemory<float>(pid_t pid, long address);
template double ReadMemory<double>(pid_t pid, long address);
template uintptr_t ReadMemory<uintptr_t>(pid_t pid, long address);

template bool WriteMemory<int>(pid_t pid, long address, const int& value);
template bool WriteMemory<long>(pid_t pid, long address, const long& value);
template bool WriteMemory<float>(pid_t pid, long address, const float& value);
template bool WriteMemory<double>(pid_t pid, long address, const double& value);

// Explicit template instantiations for GameData.h template
template int ReadMemory<int>(uintptr_t address);
template long ReadMemory<long>(uintptr_t address);
template float ReadMemory<float>(uintptr_t address);
template double ReadMemory<double>(uintptr_t address);
template uintptr_t ReadMemory<uintptr_t>(uintptr_t address);
