#include "KeyboardMonitor.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <linux/input.h>

KeyboardMonitor::KeyboardMonitor() : running(false), key_states(KEY_MAX, false) {
    std::string device_path = findKeyboardDevice();

    if (device_path.empty()) {
        std::cerr << "ERROR: No keyboard device found. "
                  << "Ensure you have permissions to read /dev/input/event*." << std::endl;
        return;
    }

    std::cout << "INFO: Found keyboard device at " << device_path << std::endl;

    keyboard_fd = open(device_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (keyboard_fd < 0) {
        std::cerr << "ERROR: Failed to open keyboard device " << device_path
                  << ". Try running with 'sudo'." << std::endl;
        return;
    }

    running = true;
    monitor_thread = std::thread(&KeyboardMonitor::monitorLoop, this);
}

KeyboardMonitor::~KeyboardMonitor() {
    running = false;
    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }
    if (keyboard_fd >= 0) {
        close(keyboard_fd);
    }
}

bool KeyboardMonitor::isKeyDown(int keyCode) const {
    if (!running || keyCode < 0 || keyCode >= key_states.size()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(key_state_mutex);
    return key_states[keyCode];
}

bool KeyboardMonitor::isMonitoring() const {
    return running;
}

void KeyboardMonitor::monitorLoop() {
    struct input_event ev;
    while (running) {
        ssize_t bytes = read(keyboard_fd, &ev, sizeof(ev));
        if (bytes == sizeof(ev)) {
            // We only care about key presses, releases, and repeats
            if (ev.type == EV_KEY && ev.code < key_states.size()) {
                std::lock_guard<std::mutex> lock(key_state_mutex);
                // ev.value == 1 (press)
                // ev.value == 0 (release)
                // ev.value == 2 (autorepeat)
                key_states[ev.code] = (ev.value != 0);
            }
        } else {
            // No event, sleep briefly to avoid busy-waiting
            usleep(10000); // 10ms
        }
    }
}

std::string KeyboardMonitor::findKeyboardDevice() {
    DIR* dir = opendir("/dev/input/by-path");
    if (!dir) {
        // Fallback to checking event* directly if by-path is not available
        dir = opendir("/dev/input");
    }
    if (!dir) {
        perror("ERROR: Could not open /dev/input");
        return "";
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        // Look for devices with "-kbd" in their name from /dev/input/by-path for reliability
        if (strstr(entry->d_name, "-kbd")) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "/dev/input/by-path/%s", entry->d_name);
            closedir(dir);
            return full_path;
        }
    }

    // Fallback if no "-kbd" device found (less reliable)
    rewinddir(dir);
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "/dev/input/%s", entry->d_name);
            int fd = open(full_path, O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            // Check if the device supports KEY_2 and KEY_A (a good sign it's a keyboard)
            unsigned long bit[EV_MAX / (sizeof(long) * 8) + 1];
            ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bit)), bit);
            if ((bit[KEY_2 / (sizeof(long) * 8)] >> (KEY_2 % (sizeof(long) * 8))) & 1 &&
                (bit[KEY_A / (sizeof(long) * 8)] >> (KEY_A % (sizeof(long) * 8))) & 1) {
                close(fd);
                closedir(dir);
                return full_path;
            }
            close(fd);
        }
    }

    closedir(dir);
    return "";
}