#ifndef KEYBOARD_MONITOR_H
#define KEYBOARD_MONITOR_H

#include <string>
#include <vector>
#include <thread>
#include <atomic>
#include <mutex>

// This header contains all the KEY_* definitions like KEY_2, KEY_A, etc.
#include <linux/input-event-codes.h>

class KeyboardMonitor {
public:
    /**
     * @brief Constructs the monitor. It will attempt to find a keyboard device
     *        and start a background monitoring thread.
     */
    KeyboardMonitor();

    /**
     * @brief Destructor. Stops the monitoring thread and cleans up.
     */
    ~KeyboardMonitor();

    // Disable copy and move semantics
    KeyboardMonitor(const KeyboardMonitor&) = delete;
    KeyboardMonitor& operator=(const KeyboardMonitor&) = delete;

    /**
     * @brief Checks if a specific key is currently being held down.
     *        This function is non-blocking and thread-safe.
     * @param keyCode The code for the key to check (e.g., KEY_2, KEY_W, KEY_ESC).
     * @return True if the key is currently pressed, false otherwise.
     */
    bool isKeyDown(int keyCode) const;

    /**
     * @brief Checks if the keyboard device was found and the monitor is running.
     * @return True if the monitor is active, false otherwise.
     */
    bool isMonitoring() const;

private:
    /**
     * @brief Scans /dev/input/event* to find a device that looks like a keyboard.
     * @return The path to the keyboard device file (e.g., "/dev/input/event3"), or an empty string if not found.
     */
    std::string findKeyboardDevice();

    /**
     * @brief The main function for the background thread. It reads events from the
     *        keyboard device and updates the key states.
     */
    void monitorLoop();

    int keyboard_fd = -1;
    std::atomic<bool> running;
    std::thread monitor_thread;

    // A mutable mutex is needed for the const isKeyDown function to lock.
    mutable std::mutex key_state_mutex;
    std::vector<bool> key_states;
};

#endif // KEYBOARD_MONITOR_H