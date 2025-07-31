#include "InputManager.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <linux/uinput.h>

// Helper to check if a bit is set in a bitmask array
bool is_bit_set(const unsigned long* bits, int bit) {
    return (bits[bit / (sizeof(long) * 8)] >> (bit % (sizeof(long) * 8))) & 1;
}

InputManager::InputManager()
    : running(true),
      key_states(KEY_MAX, false),
      delta_x(0),
      delta_y(0),
      uinput_fd(-1),
      uinput_initialized(false)
{
    setupUinput();
    scanAndOpenDevices();

    if (device_fds.empty()) {
        std::cerr << "WARNING: No input devices found or could not be opened. "
                  << "Ensure you have correct permissions for /dev/input/event*." << std::endl;
        return;
    }

    // Start a monitoring thread for each opened device
    for (int fd : device_fds) {
        monitor_threads.emplace_back(&InputManager::monitorDevice, this, fd);
    }
    std::cout << "INFO: Monitoring " << monitor_threads.size() << " input devices." << std::endl;
}

InputManager::~InputManager() {
    running = false;

    // Wait for all threads to finish
    for (auto& t : monitor_threads) {
        if (t.joinable()) {
            t.join();
        }
    }

    // Close all device file descriptors
    for (int fd : device_fds) {
        close(fd);
    }

    // Destroy the virtual device
    if (uinput_initialized) {
        ioctl(uinput_fd, UI_DEV_DESTROY);
        close(uinput_fd);
    }
}

void InputManager::scanAndOpenDevices() {
    const char* dev_dir = "/dev/input/";
    DIR* dir = opendir(dev_dir);
    if (!dir) {
        perror("ERROR: Could not open /dev/input");
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (strncmp(entry->d_name, "event", 5) == 0) {
            char dev_path[512];
            snprintf(dev_path, sizeof(dev_path), "%s%s", dev_dir, entry->d_name);

            int fd = open(dev_path, O_RDONLY | O_NONBLOCK);
            if (fd < 0) {
                continue; // Cannot open, skip
            }

            // Check device capabilities
            unsigned long ev_bits[EV_MAX / (sizeof(long) * 8) + 1] = {0};
            if (ioctl(fd, EVIOCGBIT(0, sizeof(ev_bits)), ev_bits) < 0) {
                close(fd);
                continue;
            }

            // We are interested in devices that can send key events or relative axis events
            bool is_keyboard = is_bit_set(ev_bits, EV_KEY);
            bool is_mouse = is_bit_set(ev_bits, EV_REL);

            if (is_keyboard || is_mouse) {
                char dev_name[256];
                ioctl(fd, EVIOCGNAME(sizeof(dev_name)), dev_name);
                std::cout << "INFO: Found potential device '" << dev_name << "' at " << dev_path << std::endl;
                device_fds.push_back(fd);
            } else {
                close(fd); // Not a device we care about
            }
        }
    }
    closedir(dir);
}

void InputManager::monitorDevice(int fd) {
    struct input_event ev;
    while (running) {
        ssize_t bytes = read(fd, &ev, sizeof(ev));
        if (bytes == sizeof(ev)) {
            std::lock_guard<std::mutex> lock(state_mutex);
            if (ev.type == EV_KEY && ev.code < key_states.size()) {
                // ev.value: 0=release, 1=press, 2=repeat
                key_states[ev.code] = (ev.value != 0);
            } else if (ev.type == EV_REL) {
                if (ev.code == REL_X) {
                    delta_x += ev.value;
                } else if (ev.code == REL_Y) {
                    delta_y += ev.value;
                }
            }
        } else {
            // No events, sleep briefly to avoid burning CPU
            usleep(1000); // 1ms
        }
    }
}

bool InputManager::isKeyDown(int keyCode) const {
    if (keyCode < 0 || keyCode >= key_states.size()) {
        return false;
    }
    std::lock_guard<std::mutex> lock(state_mutex);
    return key_states[keyCode];
}

void InputManager::getMouseDelta(int& dx, int& dy) {
    std::lock_guard<std::mutex> lock(state_mutex);
    dx = delta_x;
    dy = delta_y;
    // Reset after reading
    delta_x = 0;
    delta_y = 0;
}


// --- Virtual Device Implementation ---

void InputManager::setupUinput() {
    uinput_fd = open("/dev/uinput", O_WRONLY | O_NONBLOCK);
    if (uinput_fd < 0) {
        std::cerr << "ERROR: Cannot open /dev/uinput. "
                  << "Try running with 'sudo' or check permissions." << std::endl;
        return;
    }

    // We are a mouse, so we need relative events and button key events
    ioctl(uinput_fd, UI_SET_EVBIT, EV_REL);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_X);
    ioctl(uinput_fd, UI_SET_RELBIT, REL_Y);

    ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_LEFT);
    ioctl(uinput_fd, UI_SET_KEYBIT, BTN_RIGHT);
    ioctl(uinput_fd, UI_SET_KEYBIT, KEY_X);

    struct uinput_user_dev uidev;
    memset(&uidev, 0, sizeof(uidev));
    snprintf(uidev.name, UINPUT_MAX_NAME_SIZE, "Virtual Unified Mouse");
    uidev.id.bustype = BUS_USB;
    uidev.id.vendor  = 0x4242;
    uidev.id.product = 0x4242;
    uidev.id.version = 1;

    if (write(uinput_fd, &uidev, sizeof(uidev)) < 0) {
        perror("ERROR: Failed to write uinput device setup");
        close(uinput_fd);
        return;
    }

    if (ioctl(uinput_fd, UI_DEV_CREATE) < 0) {
        perror("ERROR: Failed to create uinput device");
        close(uinput_fd);
        return;
    }

    sleep(1); // Give the system a moment to recognize the new device
    uinput_initialized = true;
    std::cout << "INFO: Virtual mouse device created successfully." << std::endl;
}

void InputManager::emitUinputEvent(int type, int code, int value) {
    if (!uinput_initialized) return;

    struct input_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    ev.code = code;
    ev.value = value;
    gettimeofday(&ev.time, nullptr);

    if (write(uinput_fd, &ev, sizeof(ev)) < 0) {
        // This can happen, not always a fatal error
    }
}

void InputManager::moveMouseRelative(int dx, int dy) {
    emitUinputEvent(EV_REL, REL_X, dx);
    emitUinputEvent(EV_REL, REL_Y, dy);
    emitUinputEvent(EV_SYN, SYN_REPORT, 0); // Sync to apply changes
}

void InputManager::leftButtonDown() {
    emitUinputEvent(EV_KEY, BTN_LEFT, 1);
    emitUinputEvent(EV_SYN, SYN_REPORT, 0);
}

void InputManager::leftButtonUp() {
    emitUinputEvent(EV_KEY, BTN_LEFT, 0);
    emitUinputEvent(EV_SYN, SYN_REPORT, 0);
}

void InputManager::clickLeft() {
    leftButtonDown();
    usleep(50000); // 50ms delay between press and release
    leftButtonUp();
}

bool InputManager::isVirtualMouseInitialized() const {
    return uinput_initialized;
}

void InputManager::pressKey(int keyCode) {
    emitUinputEvent(EV_KEY, keyCode, 1); // 1 = press
    emitUinputEvent(EV_SYN, SYN_REPORT, 0);
}

void InputManager::releaseKey(int keyCode) {
    emitUinputEvent(EV_KEY, keyCode, 0); // 0 = release
    emitUinputEvent(EV_SYN, SYN_REPORT, 0);
}