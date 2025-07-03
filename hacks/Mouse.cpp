#include "Mouse.h"

#include <algorithm>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <dirent.h>
#include <linux/uinput.h>
#include <linux/input.h> // Needed for EVIOCGNAME

Mouse::Mouse() {
    uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (uinput_fd < 0) {
        std::cerr << "ERROR: Cannot open /dev/uinput. "
                  << "Try running with 'sudo' or add your user to the 'input' group." << std::endl;
        return;
    }

    // We are a mouse, so we need relative events and button key eventsa
    ioctl(uinput_fd, UI_SET_EVBIT, EV_REL);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_X);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_Y);

    ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_RIGHT);

    // Define the virtual device's properties
    struct uinput_user_dev uidev;
    std::memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Virtual C++ Mouse");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x1234;
    uidev.id.product = 0x5678;
    uidev.id.version = 1;

    // Write the device definition
    if (write(uinput_fd, &uidev, sizeof(uidev)) < 0) {
        std::cerr << "ERROR: Failed to write uinput device setup." << std::endl;
        close(uinput_fd);
        return;
    }

    // Create the device
    if (ioctl(uinput_fd, UI_DEV_CREATE) < 0) {
        std::cerr << "ERROR: Failed to create uinput device." << std::endl;
        close(uinput_fd);
        return;
    }

    // A small delay helps ensure the system has time to recognize the new device
    sleep(1);

    initialized = true;
    std::cout << "Virtual mouse device created successfully." << std::endl;
}

Mouse::~Mouse() {
    // Stop the monitoring thread first
    monitoring = false;
    if (monitor_thread.joinable()) {
        monitor_thread.join();
    }
    if (input_fd >= 0) {
        close(input_fd);
    }

    // Clean up the uinput device
    if (initialized) {
        ioctl(uinput_fd, UI_DEV_DESTROY);
        close(uinput_fd);
    }
}

bool Mouse::isInitialized() const {
    return initialized;
}

void Mouse::emit(int type, int code, int value) {
    if (!initialized) return;

    struct input_event ev;
    std::memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.code = code;
    ev.value = value;

    if (write(uinput_fd, &ev, sizeof(ev)) < 0) {
        std::cerr << "WARNING: Failed to write event to uinput device." << std::endl;
    }
}

void Mouse::moveRelative(int dx, int dy) {
    if (!initialized) return;

    emit(EV_REL, REL_X, dx);
    emit(EV_REL, REL_Y, dy);
    // After sending events, we must send a synchronization event
    emit(EV_SYN, SYN_REPORT, 0);
}

void Mouse::leftButtonDown() {
    emit(EV_KEY, BTN_LEFT, 1);
    emit(EV_SYN, SYN_REPORT, 0);
}

void Mouse::leftButtonUp() {
    emit(EV_KEY, BTN_LEFT, 0);
    emit(EV_SYN, SYN_REPORT, 0);
}

void Mouse::rightButtonDown() {
    emit(EV_KEY, BTN_RIGHT, 1);
    emit(EV_SYN, SYN_REPORT, 0);
}

void Mouse::rightButtonUp() {
    emit(EV_KEY, BTN_RIGHT, 0);
    emit(EV_SYN, SYN_REPORT, 0);
}

bool Mouse::startMonitoring(const std::string& device_name_filter) {
    if (monitoring) {
        std::cout << "INFO: Monitoring is already active." << std::endl;
        return true;
    }

    std::string device_path = findInputDevice(device_name_filter);

    if (device_path.empty()) {
        std::cerr << "ERROR: Could not find an input device matching the filter '"
                  << device_name_filter << "'." << std::endl;
        std::cerr << "       Try running 'cat /proc/bus/input/devices' to find the exact name." << std::endl;
        return false;
    }

    std::cout << "INFO: Found target device at " << device_path << ". Starting monitor." << std::endl;
    input_fd = open(device_path.c_str(), O_RDONLY | O_NONBLOCK);
    if (input_fd < 0) {
        perror("ERROR: Failed to open input device");
        return false;
    }

    monitoring = true;
    monitor_thread = std::thread(&Mouse::monitorLoop, this);
    return true;
}

bool Mouse::isKeyDown(int keyCode) const {
    if (!monitoring || keyCode < 0 || keyCode >= key_states.size()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(key_state_mutex);
    return key_states[keyCode];
}

void Mouse::monitorLoop() {
    struct input_event ev;
    while (monitoring) {
        if (read(input_fd, &ev, sizeof(ev)) == sizeof(ev)) {
            if (ev.type == EV_KEY && ev.code < key_states.size()) {
                std::lock_guard<std::mutex> lock(key_state_mutex);
                key_states[ev.code] = (ev.value != 0); // 1 for press, 2 for repeat, 0 for release
            }
        } else {
            usleep(5000); // 5ms sleep when no events
        }
    }
}

std::string Mouse::findInputDevice(const std::string& filter) {
    DIR* dir = opendir("/dev/input");
    if (!dir) {
        perror("ERROR: Could not open /dev/input");
        return "";
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            char full_path[1024];
            snprintf(full_path, sizeof(full_path), "/dev/input/%s", entry->d_name);
            int fd = open(full_path, O_RDONLY | O_NONBLOCK);
            if (fd < 0) continue;

            char device_name[256];
            if (ioctl(fd, EVIOCGNAME(sizeof(device_name)), device_name) < 0) {
                close(fd);
                continue;
            }

            // Check if the device name contains our filter string (case-insensitive)
            // std::cout << "DEBUG: Checking device: " << device_name << std::endl;
            std::string name_str(device_name);
            // std::cout << "DEBUG: Device name is: " << name_str << std::endl;
            std::string filter_lower = filter;
            // std::cout << "DEBUG: Filter is: " << filter_lower << std::endl;
            std::transform(name_str.begin(), name_str.end(), name_str.begin(), ::tolower);
            // std::cout << "DEBUG: Device name (lowercase) is: " << name_str << std::endl;
            std::transform(filter_lower.begin(), filter_lower.end(), filter_lower.begin(), ::tolower);
            // std::cout << "DEBUG: Filter (lowercase) is: " << filter_lower << std::endl;

            if (name_str.find(filter_lower) != std::string::npos) {
                // It's a match! Now, let's confirm it has keyboard-like capabilities.
                unsigned long bit[EV_MAX / (sizeof(long) * 8) + 1];
                ioctl(fd, EVIOCGBIT(EV_KEY, sizeof(bit)), bit);
                // We check for a common numpad key to be sure it's the right part of the mouse.
                if ((bit[KEY_KP1 / (sizeof(long) * 8)] >> (KEY_KP1 % (sizeof(long) * 8))) & 1) {
                    close(fd);
                    closedir(dir);
                    return full_path;
                }
            }
            close(fd);
        }
    }

    closedir(dir);
    return "";
}